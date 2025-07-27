#pragma once
#include "proxybase.hpp"

class PDProxy : public ProxyBase
{
public:
    PDProxy(unsigned short port);
    ~PDProxy() override = default;
    std::string udp_packet_to_json(std::span<const char> data) override;
    const char* module_name() const override { return "PDProxy"; }
};
