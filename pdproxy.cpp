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

    // Parse the PDP panel state
    pdp_panel_state panel_state;
    memcpy(&panel_state, data.data() + sizeof(panel_packet_header), sizeof(pdp_panel_state));

    int mode = (panel_state.ps_psw >> 14) & 0x3; // PSW bits 15 and 14: Processor mode
    bool addr22 = (panel_state.ps_mmr3 & (1 << 4)) != 0; // MMR3 bit 4: 22-bit addressing
    bool addr18 = (panel_state.ps_mmr0 & (1 << 0)) && !addr22; // MMR0 bit 0: 18-bit addressing if not in 22-bit mode

    nlohmann::json json;
    json["address"]       = panel_state.ps_address & 0x3fffff; // Mask to 22 bits
    json["data"]          = panel_state.ps_data;

    json["parity_error"]  = (panel_state.ps_mser & (
        (1 << 7) | // CACHE HB DATA PARITY ERROR
        (1 << 6) | // CACHE LB DATA PARITY ERROR
        (1 << 5) | // CACHE CPU TAG PARITY ERROR
        (1 << 4)   // CACHE DMA TAG PARITY ERROR
    )) != 0;

    json["address_error"] = (panel_state.ps_cpu_err & (
        (1 << 6) | // ADDRESS ERROR
        (1 << 5)   // NONEXISTENT MEMORY
    )) != 0;

    json["user_mode"]     = (mode == 3);
    json["super_mode"]    = (mode == 1);
    json["kernel_mode"]   = (mode == 0);

    json["addr16"]        = !(addr18 || addr22);
    json["addr18"]        = addr18;
    json["addr22"]        = addr22;

    return json.dump();
}
