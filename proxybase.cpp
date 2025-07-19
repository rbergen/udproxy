#include "proxybase.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <chrono>

ProxyBase::ProxyBase(unsigned short port)
    : port(port)
    , stop_flag(false)
    , udp_thread()
    , server(port, "0.0.0.0")
{
}

ProxyBase::~ProxyBase()
{
    stop_flag = true;
    server.stop();
    if (udp_thread.joinable())
        udp_thread.join();
}

void ProxyBase::run()
{
    ix::initNetSystem();

    log_info("Starting WebSocket server on port %u", server.getPort());

    server.setOnConnectionCallback([this](std::weak_ptr<ix::WebSocket> ws, std::shared_ptr<ix::ConnectionState> state) {
        if (auto s = ws.lock())
        {
            s->setOnMessageCallback([](const ix::WebSocketMessagePtr&) {});
        }

        if (state)
        {
            log_info("WebSocket client connected: %s:%d", state->getRemoteIp().c_str(), state->getRemotePort());
        }
    });

    auto [success, errorMsg] = server.listen();
    if (!success)
    {
        log_error("WebSocket server listen failed: %s", errorMsg.c_str());
        return;
    }

    server.start();
    log_info("WebSocket server started");

    udp_thread = std::thread([this]() { udp_loop(); });

    server.wait();
    log_info("WebSocket server stopped");

    if (udp_thread.joinable())
        udp_thread.join();

    ix::uninitNetSystem();
}

void ProxyBase::stop()
{
    stop_flag = true;
    server.stop();
}

void ProxyBase::udp_loop()
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
    while (!stop_flag)
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
            continue;

        for (auto& client : server.getClients())
        {
            if (client && client->getReadyState() == ix::ReadyState::Open)
                client->send(json);
        }
    }

    close(sock);
    log_info("UDP socket closed");
}
