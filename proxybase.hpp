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

class ProxyBase : public Loggable {
public:
    ProxyBase(unsigned short port);
    virtual ~ProxyBase();
    void run(); // blocks
    void stop();
    virtual std::string udp_packet_to_json(std::span<const char> data) = 0;
    unsigned short get_proxy_port() const { return port; }
    void udp_loop();

protected:
    unsigned short port;
    std::atomic<bool> stop_flag;
    std::thread udp_thread;
    ix::WebSocketServer server;
};
