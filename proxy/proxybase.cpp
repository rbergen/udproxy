#include "proxybase.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <chrono>

ProxyBase::ProxyBase(unsigned short port)
    : port(port)
{
    ws_server.loglevel(crow::LogLevel::Warning); // Set log level to Info
}

ProxyBase::~ProxyBase()
{
    ws_server.stop();
    udp_thread.request_stop();
}

void ProxyBase::run()
{
    log_info("Starting WebSocket server on port %u", port);

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

    udp_thread = std::jthread([&](std::stop_token stop_token) { udp_loop(stop_token); });

    server_future = ws_server.port(port).multithreaded().run_async();
    if (ws_server.wait_for_server_start() != std::cv_status::no_timeout)
    {
        log_error("Failed to start WebSocket server on port %u", port);
        return;
    }

    log_info("WebSocket server started");

    server_future.wait();
    log_info("WebSocket server stopped");

    udp_thread.request_stop();
    if (udp_thread.joinable())
        udp_thread.join();
}

void ProxyBase::stop()
{
    ws_server.stop();
    udp_thread.request_stop();
}

void ProxyBase::udp_loop(std::stop_token stop_token)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        log_error("Failed to create UDP socket: %s", strerror(errno));
        return;
    }

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
    while (!stop_token.stop_requested())
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
