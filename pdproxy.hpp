#pragma once
#include <string>
#include <span>
#include "json.hpp"
#include <ixwebsocket/IXWebSocketServer.h>
#include <ixwebsocket/IXNetSystem.h>
#include <thread>
#include <atomic>
#include <vector>

class PDProxy
{
public:
    PDProxy(unsigned short udp_port, unsigned short ws_port);
    ~PDProxy();
    void run(); // blocks
    std::string udp_packet_to_json(std::span<const char> data);
    void graceful_shutdown();
private:
    void udp_loop();
    unsigned short udp_port;
    ix::WebSocketServer server;
    std::thread udp_thread;
    std::atomic<bool> stop_flag;
};
