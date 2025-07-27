#pragma once

#include <cstdint>

#pragma pack(push, 1)

struct panel_packet_header
{
    uint16_t pp_byte_count;    /* Size of the panel_state payload */
    uint32_t pp_byte_flags;    /* Panel type flags (PANEL_PDP1170, PANEL_VAX, etc.) */
};

#pragma pack(pop)