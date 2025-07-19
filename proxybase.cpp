#include "proxybase.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <chrono>

ProxyBase::ProxyBase(unsigned short proxy_port, unsigned short http_port, std::string content_dir)
    : proxy_port(proxy_port)
    , http_port(http_port)
    , content_dir(std::move(content_dir))
    , stop_flag(false)
    , http_server(nullptr)
    , udp_thread()
    , http_thread()
    , server(proxy_port, "0.0.0.0")
{
}

ProxyBase::~ProxyBase()
{
    stop_flag = true;
    server.stop();
    if (http_server)
        http_server->stop();
    if (udp_thread.joinable())
        udp_thread.join();
    if (http_thread.joinable())
        http_thread.join();
    http_server.reset();
}

void ProxyBase::log_info(const char* fmt, ...) const {
    va_list args;
    va_start(args, fmt);
    log_message_va(module_name(), "INFO", fmt, args);
    va_end(args);
}

void ProxyBase::log_error(const char* fmt, ...) const {
    va_list args;
    va_start(args, fmt);
    log_message_va(module_name(), "ERROR", fmt, args);
    va_end(args);
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
    auto res = server.listen();
    if (!res.first)
    {
        log_error("WebSocket server listen failed: %s", res.second.c_str());
        return;
    }
    server.start();
    log_info("WebSocket server started");
    udp_thread = std::thread([this]() { udp_loop(); });
    http_thread = std::thread([this]() { serve_http(); });
    server.wait();
    if (udp_thread.joinable())
        udp_thread.join();
    if (http_thread.joinable())
        http_thread.join();
    log_info("WebSocket server stopped");
    ix::uninitNetSystem();
}

void ProxyBase::stop()
{
    stop_flag = true;
    server.stop();
    if (http_server)
        http_server->stop();
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
    addr.sin_port = htons(proxy_port);
    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0)
    {
        log_error("Failed to bind UDP socket: %s", strerror(errno));
        close(sock);
        return;
    }
    log_info("UDP socket listening on port %u", proxy_port);
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
    log_info("UDP socket closed");
    close(sock);
}

void ProxyBase::serve_http()
{
    http_server = std::make_unique<httplib::Server>();
    http_server->set_mount_point("/", content_dir.c_str());
    log_info("HTTP server serving directory '%s' on port %u", content_dir.c_str(), http_port);
    http_server->listen("0.0.0.0", http_port);
    log_info("HTTP server stopped");
    http_server.reset();
}
