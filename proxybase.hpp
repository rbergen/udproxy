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
#include <ctime>
#include <cstdarg>
#include <cstdio>

inline void log_message(const char* module, const char* level, const char* fmt, ...) {
    FILE* out = (level[0] == 'E') ? stderr : stdout;
    char ts[32];
    std::time_t t = std::time(nullptr);
    std::strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    std::fprintf(out, "[%s] [%s] [%s] ", ts, level, module);
    va_list args;
    va_start(args, fmt);
    std::vfprintf(out, fmt, args);
    va_end(args);
    std::fprintf(out, "\n");
}

#define LOG_INFO(module, fmt, ...) log_message(module, "INFO", fmt, ##__VA_ARGS__)
#define LOG_ERROR(module, fmt, ...) log_message(module, "ERROR", fmt, ##__VA_ARGS__)

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
