#pragma once
#include <string>
#include <span>
#include <atomic>
#include <thread>
#include <memory>
#include <vector>
#include <ixwebsocket/IXWebSocketServer.h>
#include "logging.hpp"
#include "types.hpp"
#define CROW_ENABLE_COMPRESSION 1
#include "crow_all.h"

class ProxyBase : public Loggable {
public:
    ProxyBase(unsigned short port);
    virtual ~ProxyBase();
    void run(); // blocks
    void stop();
    virtual std::string udp_packet_to_json(std::span<const char> data) = 0;
    unsigned short get_proxy_port() const { return port; }

    // Non-copyable, non-movable
    ProxyBase(const ProxyBase&) = delete;
    ProxyBase& operator=(const ProxyBase&) = delete;
    ProxyBase(ProxyBase&&) = delete;
    ProxyBase& operator=(ProxyBase&&) = delete;

protected:
    template<typename T>
    bool get_panel_state(std::span<const char> data, T& panel_state)
    {
        // Check minimum size for header
        if (data.size() < sizeof(panel_packet_header))
        {
            log_error("Packet too small for header: %zu bytes (need %zu)",
                    data.size(), sizeof(panel_packet_header));
            return false;
        }

        // Parse the header
        panel_packet_header header;
        memcpy(&header, data.data(), sizeof(panel_packet_header));

        // Check if we have enough data for the complete packet
        size_t expected_total_size = sizeof(panel_packet_header) + header.pp_byte_count;
        if (data.size() < expected_total_size)
        {
            log_error("Packet too small: %zu bytes (need %zu total, header=%zu + payload=%u)",
                    data.size(), expected_total_size, sizeof(panel_packet_header), header.pp_byte_count);
            return false;
        }

        // Check if the payload size matches the PDP panel state size
        if (header.pp_byte_count != sizeof(T))
        {
            log_error("Invalid payload size: %u bytes (expected %zu for panel state)",
                    header.pp_byte_count, sizeof(T));
            return false;
        }

        memcpy(&panel_state, data.data() + sizeof(panel_packet_header), sizeof(T));
        return true;
    }

    template<typename T, size_t N = (sizeof(T) * 2)>
    std::string to_hex(T value)
    {
        static const char* digits = "0123456789ABCDEF";
        std::string result(N, '0');

        for (size_t i = N - 1; i >= 0 && value; --i)
        {
            result[i] = digits[value & 0xF];
            value >>= 4;
        }

        return result;
    }

    unsigned short port;
private:
    void udp_loop(std::stop_token stop_token);

    std::jthread udp_thread;
    crow::SimpleApp ws_server;
    std::future<void> server_future;
    std::set<crow::websocket::connection*> ws_clients;
    std::mutex ws_clients_mutex;
};
