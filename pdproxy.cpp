#include "pdproxy.hpp"
#include <ixwebsocket/IXWebSocketServer.h>
#include <ixwebsocket/IXNetSystem.h>
#include <thread>
#include <atomic>
#include <vector>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <csignal>

namespace {
    static PDProxy* g_pdproxy_instance = nullptr;
    void signal_handler(int)
    {
        if (g_pdproxy_instance) g_pdproxy_instance->graceful_shutdown();
    }
}

void PDProxy::graceful_shutdown()
{
    stop_flag = true;
    server.stop();
}

PDProxy::PDProxy(unsigned short udp_port, unsigned short ws_port)
    : udp_port(udp_port), server(ws_port), stop_flag(false)
{
}

PDProxy::~PDProxy()
{
    stop_flag = true;
    if (udp_thread.joinable())
        udp_thread.join();
}

void PDProxy::run()
{
    ix::initNetSystem();

    // Register signal handler for graceful shutdown
    g_pdproxy_instance = this;
    std::signal(SIGINT, signal_handler);

    server.start();

    udp_thread = std::thread([this]() { udp_loop(); });
    server.wait();

    stop_flag = true;

    if (udp_thread.joinable())
        udp_thread.join();

    ix::uninitNetSystem();
    g_pdproxy_instance = nullptr;
}

void PDProxy::udp_loop()
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        return;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(udp_port);

    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0)
    {
        close(sock);
        return;
    }

    std::vector<char> buffer(2048);
    while (!stop_flag)
    {
        ssize_t n = recv(sock, buffer.data(), buffer.size(), 0);
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

    close(sock);
}

std::string PDProxy::udp_packet_to_json(std::span<const char> data)
{
    if (data.size() != 16)
        return "";

    uint32_t address;
    uint16_t ps_data, ps_psw, ps_mser, ps_cpu_err, ps_mmr0, ps_mmr3;

    memcpy(&address,  &data[0],  4);
    memcpy(&ps_data,  &data[4],  2);
    memcpy(&ps_psw,   &data[6],  2);
    memcpy(&ps_mser,  &data[8],  2);
    memcpy(&ps_cpu_err, &data[10], 2);
    memcpy(&ps_mmr0,  &data[12], 2);
    memcpy(&ps_mmr3,  &data[14], 2);

    int mode = (ps_psw >> 14) & 0x3;
    bool is_user_mode = (mode == 3);
    bool is_super_mode = (mode == 1);
    bool is_kernel_mode = (mode == 0);
    bool data_on = (ps_psw & (1 << 13));
    bool addr22 = (ps_mmr3 & (1 << 4));
    bool addr18 = !addr22 && (ps_mmr3 & (1 << 5));
    bool addr16 = !(addr22 || addr18);
    bool prog_phy = !(ps_mmr0 & 1);

    nlohmann::json json;
    json["address"]       = address & 0x3fffff; // Mask to 22 bits
    json["data"]          = ps_data;
    json["parity_error"]  = (ps_mser & 0x7380) != 0;
    json["address_error"] = (ps_cpu_err & 0xfc00) != 0;
    json["run"]           = true;
    json["pause"]         = false;
    json["master"]        = true;
    json["user_mode"]     = is_user_mode;
    json["super_mode"]    = is_super_mode;
    json["kernel_mode"]   = is_kernel_mode;
    json["data_on"]       = data_on;
    json["addr16"]        = addr16;
    json["addr18"]        = addr18;
    json["addr22"]        = addr22;
    json["user_d"]        = !prog_phy && is_user_mode && data_on;
    json["user_i"]        = !prog_phy && is_user_mode && !data_on;
    json["super_d"]       = !prog_phy && is_super_mode && data_on;
    json["super_i"]       = !prog_phy && is_super_mode && !data_on;
    json["kernel_d"]      = !prog_phy && is_kernel_mode && data_on;
    json["kernel_i"]      = !prog_phy && is_kernel_mode && !data_on;
    json["cons_phy"]      = false;
    json["prog_phy"]      = prog_phy;
    json["parity_high"]   = !!(ps_mser & (1 << 9));
    json["parity_low"]    = !!(ps_mser & (1 << 8));

    return json.dump();
}
