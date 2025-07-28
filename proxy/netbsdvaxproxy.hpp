#pragma once
#include "proxybase.hpp"

class NetBSDVAXProxy : public ProxyBase
{
public:
    NetBSDVAXProxy(unsigned short port);
    ~NetBSDVAXProxy() override = default;
    std::string udp_packet_to_json(std::span<const char> data) override;
    const char* module_name() const override { return "NetBSDVAXProxy"; }
};
