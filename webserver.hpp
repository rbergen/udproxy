#pragma once
#include "logging.hpp"
#include "httplib.h"
#include <string>
#include <memory>
#include <thread>

class WebServer : public Loggable {
public:
    WebServer(unsigned short port, std::string content_dir);
    ~WebServer();
    void run(); // blocks
    void stop();
    const char* module_name() const override;
    void add_proxy_port(const std::string& proxy_name, unsigned short port);
protected:
    unsigned short port;
    std::string content_dir;
    std::unique_ptr<httplib::Server> server;
    std::thread server_thread;
    std::atomic<bool> running;
    std::map<std::string, unsigned short> proxy_ports;
};
