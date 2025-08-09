#include "proxybase.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <chrono>
#include <cerrno>

ProxyBase::ProxyBase(unsigned short port)
    : port(port)
{
    ws_server.loglevel(crow::LogLevel::Warning); // Set log level to Warning

    CROW_WEBSOCKET_ROUTE(ws_server, "/")
    .onopen([&](crow::websocket::connection& conn)
    {
        {
            std::lock_guard guard(ws_clients_mutex);
            ws_clients.insert(&conn);
        }

        log_info("WebSocket client connected: %s", conn.get_remote_ip().c_str());
    })
    .onclose([&](crow::websocket::connection& conn, const std::string&, uint16_t)
    {
        std::lock_guard guard(ws_clients_mutex);
        ws_clients.erase(&conn);
    })
    .onerror([&](crow::websocket::connection& conn, const std::string& msg)
    {
        log_error("WebSocket error from %s: %s", conn.get_remote_ip().c_str(), msg.c_str());
        std::lock_guard guard(ws_clients_mutex);
        ws_clients.erase(&conn);
    });
}

ProxyBase::~ProxyBase()
{
    ws_server.stop();
    stop_requested.store(true);
    if (udp_thread.joinable())
        udp_thread.join();
}

void ProxyBase::run()
{
    log_info("Starting WebSocket server on port %u", port);
    server_future = ws_server.port(port).multithreaded().run_async();
    if (ws_server.wait_for_server_start() != std::cv_status::no_timeout)
    {
        log_error("Failed to start WebSocket server on port %u", port);
        ws_server.stop();
        try { server_future.get(); } catch (...) { /* swallow */ }
        return;
    }

    log_info("WebSocket server started");

    udp_thread = std::thread([&]() { udp_loop(); });

    server_future.wait();
    log_info("WebSocket server stopped");
    try { server_future.get(); } catch (...) { /* swallow */ }

    stop_requested.store(true);
    if (udp_thread.joinable())
        udp_thread.join();
}

void ProxyBase::stop()
{
    ws_server.stop();
    stop_requested.store(true);
}

void ProxyBase::udp_loop()
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        log_error("Failed to create UDP socket: %s", strerror(errno));
        return;
    }

    // Non-blocking and set reuseaddr
    int reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
        log_error("Failed to set SO_REUSEADDR on UDP socket: %s", strerror(errno));

        fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0)
    {
        log_error("Failed to bind UDP socket: %s", strerror(errno));
        close(sock);
        return;
    }
    log_info("UDP socket listening on port %u", port);

    std::vector<char> buffer(2048);
    while (!stop_requested.load())
    {
        ssize_t n = recv(sock, buffer.data(), buffer.size(), 0);
        if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            log_error("UDP recv error: %s", strerror(errno));
            break;
        }

        if (n == 0)
            continue;

        std::string json = udp_packet_to_json(std::span<const char>(buffer.data(), n));
        if (json.empty())
        {
            log_error("Failed to parse UDP packet (%zd bytes) - packet cannot be processed", n);
            continue;
        }

        {
            std::lock_guard guard(ws_clients_mutex);
            for (auto *client : ws_clients)
                client->send_text(json);
        }
    }

    close(sock);
    log_info("UDP socket closed");
}
