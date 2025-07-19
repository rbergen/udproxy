#pragma once
#include "proxybase.hpp"

class PDProxy : public ProxyBase {
public:
    PDProxy(unsigned short proxy_port, unsigned short http_port, std::string content_dir);
    ~PDProxy() override = default;
    std::string udp_packet_to_json(std::span<const char> data) override;
    const char* module_name() const override { return "PDProxy"; }
};
