#pragma once
#include "proxybase.hpp"



class AMD64Proxy : public ProxyBase
{
public:
    AMD64Proxy(unsigned short port);
    ~AMD64Proxy() override = default;
    std::string udp_packet_to_json(std::span<const char> data) override;
    const char* module_name() const override { return "AMD64Proxy"; }
};
