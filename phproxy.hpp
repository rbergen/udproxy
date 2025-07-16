#pragma once
#include <system_error>
#include "asio.hpp"
#include "websocketpp/config/asio_no_tls.hpp"
#include "websocketpp/server.hpp"
#include <string>
#include <memory>
#include <span>
#include "json.hpp" // nlohmann/json single header

// Forward declaration to minimize header dependencies
class PhProxy {
public:
    PhProxy(unsigned short udp_port, unsigned short ws_port);
    virtual ~PhProxy();

    void run(); // blocks

protected:
    // Override this method to implement your UDP→JSON mapping logic
    virtual std::string udp_packet_to_json(std::span<const char> data) = 0;

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl; // PIMPL idiom to keep header minimal
};
