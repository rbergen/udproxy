#include "phproxy.hpp"
#include "json.hpp" // nlohmann/json single header
#include <iostream>

// Example: override udp_packet_to_json for custom parsing
class PDP11PhProxy : public PhProxy {
public:
    using PhProxy::PhProxy;

protected:
    std::string udp_packet_to_json(const char* data, std::size_t len) override {
        // TODO: Parse PDP-11 status data and construct nlohmann::json object.
        // Placeholder:
        nlohmann::json j;
        j["cpu_status_hex"] = nlohmann::json::binary({data, data+len});
        // j["whatever"] = ...;
        return j.dump();
    }
};

int main() {
    unsigned short udp_port = 4000;
    unsigned short ws_port = 9002;
    PDP11PhProxy proxy(udp_port, ws_port);
    proxy.run();
    return 0;
}
