#include "pdproxy.hpp"
#include <cstring>

// Packet structure definitions - use tight packing to match network protocol
#pragma pack(push, 1)

struct panel_packet_header {
    uint16_t pp_byte_count;    /* Size of the panel_state payload */
    uint32_t pp_byte_flags;    /* Panel type flags (PANEL_PDP1170, PANEL_VAX, etc.) */
};

struct pdp_panel_state {
    uint32_t ps_address;    /* panel switches - 32-bit address */
    uint16_t ps_data;       /* panel lamps - 16-bit data */
    uint16_t ps_psw;        /* processor status word */
    uint16_t ps_mser;       /* machine status register */
    uint16_t ps_cpu_err;    /* CPU error register */
    uint16_t ps_mmr0;       /* memory management register 0 */
    uint16_t ps_mmr3;       /* memory management register 3 */
};

struct pdp_panel_packet {
    panel_packet_header header;
    pdp_panel_state panel_state;
};

#pragma pack(pop)

PDProxy::PDProxy(unsigned short port)
    : ProxyBase(port)
{
    // Debug: Print structure sizes to verify pragma pack worked
    log_info("Structure sizes: header=%zu bytes, panel_state=%zu bytes, total_packet=%zu bytes",
             sizeof(panel_packet_header), sizeof(pdp_panel_state), sizeof(pdp_panel_packet));
}

std::string PDProxy::udp_packet_to_json(std::span<const char> data)
{
    // Check minimum size for header
    if (data.size() < sizeof(panel_packet_header))
    {
        log_error("Packet too small for header: %zu bytes (need %zu)",
                  data.size(), sizeof(panel_packet_header));
        return "";
    }

    // Parse the header
    panel_packet_header header;
    memcpy(&header, data.data(), sizeof(panel_packet_header));

    log_info("Packet header: byte_count=%u, byte_flags=0x%08x",
             header.pp_byte_count, header.pp_byte_flags);

    // Check if we have enough data for the complete packet
    size_t expected_total_size = sizeof(panel_packet_header) + header.pp_byte_count;
    if (data.size() < expected_total_size)
    {
        log_error("Packet too small: %zu bytes (need %zu total, header=%zu + payload=%u)",
                  data.size(), expected_total_size, sizeof(panel_packet_header), header.pp_byte_count);
        return "";
    }

    // Check if the payload size matches the PDP panel state size
    if (header.pp_byte_count != sizeof(pdp_panel_state))
    {
        log_error("Invalid payload size: %u bytes (expected %zu for PDP panel state)",
                  header.pp_byte_count, sizeof(pdp_panel_state));
        return "";
    }

    log_info("Successfully parsed packet header, extracting PDP panel state");

    // Parse the PDP panel state
    pdp_panel_state panel_state;
    memcpy(&panel_state, data.data() + sizeof(panel_packet_header), sizeof(pdp_panel_state));

    int mode = (panel_state.ps_psw >> 14) & 0x3;
    bool addr22 = (panel_state.ps_mmr3 & (1 << 4));
    bool addr18 = !addr22 && (panel_state.ps_mmr3 & (1 << 5));

    nlohmann::json json;
    json["address"]       = panel_state.ps_address & 0x3fffff; // Mask to 22 bits
    json["data"]          = panel_state.ps_data;
    json["parity_error"]  = (panel_state.ps_mser & 0x8000) != 0;
    json["address_error"] = (panel_state.ps_cpu_err & 0x2000) != 0;
    json["run"]           = true;
    json["pause"]         = false;
    json["master"]        = true;
    json["user_mode"]     = (mode == 3);
    json["super_mode"]    = (mode == 1);
    json["kernel_mode"]   = (mode == 0);
    json["data_on"]       = (panel_state.ps_psw & (1 << 13)) != 0;
    json["addr16"]        = !(addr18 || addr22);
    json["addr18"]        = addr18;
    json["addr22"]        = addr22;

    log_info("Generated JSON for PDP state: address=0x%06x, data=0x%04x, psw=0x%04x",
             panel_state.ps_address & 0x3fffff, panel_state.ps_data, panel_state.ps_psw);

    return json.dump();
}
