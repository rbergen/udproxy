#pragma once
#include "logging.hpp"
#include <string>
#include <memory>
#include <thread>
#define CROW_ENABLE_COMPRESSION 1
#include "crow_all.h"

class WebServer : public Loggable {
public:
    WebServer(unsigned short port, std::string content_dir);
    ~WebServer();
    void run(); // blocks
    void stop();
    const char* module_name() const override { return "WebServer"; };
    void add_proxy_port(const std::string& proxy_name, unsigned short port);
protected:
    unsigned short port;
    std::string content_dir;
    crow::SimpleApp server;
    std::map<std::string, unsigned short> proxy_ports;
};
