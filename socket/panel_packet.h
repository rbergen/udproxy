/*
 * panel_packet.h - Common panel packet header structure
 * Shared across all platform implementations
 */

#ifndef PANEL_PACKET_H
#define PANEL_PACKET_H

#include <stdint.h>

/* Panel type enumeration for packet flags */
typedef enum {
    PANEL_PDP1170 = 1,      /* PDP-11/70 2.11BSD panel */
    PANEL_VAX = 2,          /* VAX NetBSD panel */
    PANEL_NETBSDX64 = 3,    /* NetBSD x64 panel */
    PANEL_MACOS = 4,        /* macOS panel */
    PANEL_LINUXX64 = 5      /* Linux x64 panel */
} panel_type_t;

/* Common packet header structure */
#pragma pack(push, 1)
struct panel_packet_header {
    uint16_t pp_byte_count;    /* Size of the panel_state payload */
    uint32_t pp_byte_flags;    /* Panel type flags (PANEL_PDP1170, PANEL_VAX, etc.) */
};
#pragma pack(pop)
#endif /* PANEL_PACKET_H */
