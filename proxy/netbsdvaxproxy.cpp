#include "netbsdvaxproxy.hpp"
#include <cstring>
#include "../socket/panel_packet.h"

// Packet structure definitions - use tight packing to match network protocol
#pragma pack(push, 1)

struct netbsdvax_panel_state
{
    uint32_t ps_address;    /* panel switches - 32-bit address */
    uint32_t ps_data;       /* panel lamps - 32-bit data */
};

struct netbsdvax_panel_packet
{
    panel_packet_header header;
    netbsdvax_panel_state panel_state;
};

#pragma pack(pop)

NetBSDVAXProxy::NetBSDVAXProxy(unsigned short port)
    : ProxyBase(port)
{
    // Debug: Print structure sizes to verify pragma pack worked
    log_info("Structure sizes: header=%zu bytes, panel_state=%zu bytes, total_packet=%zu bytes",
             sizeof(panel_packet_header), sizeof(netbsdvax_panel_state), sizeof(netbsdvax_panel_packet));
}

std::string NetBSDVAXProxy::udp_packet_to_json(std::span<const char> data)
{
    // Parse the NetBSD VAX panel state
    netbsdvax_panel_state panel_state;
    if (!get_panel_state(data, PANEL_VAX, panel_state))
        return "";

    crow::json::wvalue json;
    json["address"] = panel_state.ps_address; // Full 32-bit address
    json["data"]    = panel_state.ps_data;    // Full 32-bit data

    return json.dump();
}
