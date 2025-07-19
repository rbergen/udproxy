#pragma once
#include <string>
#include <span>
#include <atomic>
#include <thread>
#include <memory>
#include <vector>
#include "json.hpp"
#include "httplib.h"
#include <ixwebsocket/IXWebSocketServer.h>
#include "logging.hpp"

class ProxyBase {
public:
    ProxyBase(unsigned short proxy_port, unsigned short http_port, std::string content_dir);
    virtual ~ProxyBase();
    void run(); // blocks
    void stop();
    virtual std::string udp_packet_to_json(std::span<const char> data) = 0;
    virtual const char* module_name() const = 0;
    void udp_loop();
    void serve_http();
    unsigned short get_proxy_port() const { return proxy_port; }
    unsigned short get_http_port() const { return http_port; }
protected:
    void log_info(const char* fmt, ...) const;
    void log_error(const char* fmt, ...) const;
    unsigned short proxy_port;
    unsigned short http_port;
    std::string content_dir;
    std::atomic<bool> stop_flag;
    std::unique_ptr<httplib::Server> http_server;
    std::thread udp_thread;
    std::thread http_thread;
    ix::WebSocketServer server;
};
