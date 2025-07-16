#pragma once
#include <system_error>
#include "asio.hpp"
#include "websocketpp/config/asio_no_tls.hpp"
#include "websocketpp/server.hpp"
#include <string>
#include <memory>
#include <span>
#include "json.hpp" // nlohmann/json single header

class PDProxy
{
public:
    PDProxy(unsigned short udp_port, unsigned short ws_port);
    ~PDProxy();

    void run(); // blocks

protected:
    // Implement UDP→JSON mapping logic
    virtual std::string udp_packet_to_json(std::span<const char> data);

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};
