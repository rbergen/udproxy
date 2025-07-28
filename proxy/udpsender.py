#!/usr/bin/env python3
# udpsender.py - A simple UDP sender for PDP-11, AMD64, and NetBSD/VAX panel packets

import socket
import struct
import readline  # allows up/down arrow history

# Packet definitions
PACKET_TYPES = {
    "pdp11": {
        "desc": "PDP-11",
        "udp_port": 4000,
        "fields": {
            "pp_byte_count": 16,
            "pp_byte_flags": 1,
            "ps_address": 0x15555,
            "ps_data": 0xAAAA,
            "ps_psw": 0x0200,
            "ps_mser": 0x0001,
            "ps_cpu_err": 0x0000,
            "ps_mmr0": 0x7000,
            "ps_mmr3": 0x0003,
        },
        "format": "<HI I 6H"
    },
    "amd64": {
        "desc": "NetBSD/AMD64",
        "udp_port": 4001,
        "fields": {
            "pp_byte_count": (26 * 8),  # 26*8
            "pp_byte_flags": 1,
            "cf_rdi": 0, "cf_rsi": 0, "cf_rdx": 0, "cf_rcx": 0, "cf_r8": 0, "cf_r9": 0,
            "cf_r10": 0, "cf_r11": 0, "cf_r12": 0, "cf_r13": 0, "cf_r14": 0, "cf_r15": 0,
            "cf_rbp": 0, "cf_rbx": 0, "cf_rax": 0, "cf_gs": 0, "cf_fs": 0, "cf_es": 0, "cf_ds": 0,
            "cf_trapno": 0, "cf_err": 0, "cf_rip": 0, "cf_cs": 0, "cf_rflags": 0, "cf_rsp": 0, "cf_ss": 0,
        },
        "format": "<HI" + "Q"*26
    },
    "vax": {
        "desc": "NetBSD/VAX",
        "udp_port": 4002,
        "fields": {
            "pp_byte_count": 8,   # 4 (header) + 4 (state)
            "pp_byte_flags": 1,
            "ps_address": 0x1000,
            "ps_data": 0x2000,
        },
        # panel_packet_header: uint16_t pp_byte_count, uint32_t pp_byte_flags
        # netbsdvax_panel_state: uint32_t ps_address, uint32_t ps_data
        # But types.hpp shows header as uint16_t, uint32_t (6 bytes), but netbsdvax_panel_packet in .cpp expects header + state (2+4 + 4+4 = 14 bytes? But code uses 4+4 for state, so total 6+8=14? We'll use <HI 2I for 2+4+4+4=14 bytes)
        "format": "<HI 2I"
    }
}

UDP_IP = "127.0.0.1"

def build_packet(fields, fmt):
    try:
        values = [fields[k] for k in fields]
        return struct.pack(fmt, *values)
    except Exception as e:
        print(f"Error building packet: {e}")
        return b""

def print_packet(fields):
    print("Current packet field values:")
    for k, v in fields.items():
        print(f"  {k} = 0x{v:X}")

def parse_value(val):
    return int(val, 0)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

print("UDP Panel Packet Sender")
print(f"Packet types: " + ", ".join(PACKET_TYPES.keys()))

while True:
    print("\nSelect packet type:")
    for i, key in enumerate(PACKET_TYPES):
        print(f"  {i+1}. {PACKET_TYPES[key]['desc']} ({key})")
    print("  q. Quit")
    sel = input("> ").strip().lower()
    if sel in ("q", "quit", "exit"):
        break
    if sel.isdigit() and 1 <= int(sel) <= len(PACKET_TYPES):
        pkt_key = list(PACKET_TYPES.keys())[int(sel)-1]
    elif sel in PACKET_TYPES:
        pkt_key = sel
    else:
        print("Unknown selection")
        continue

    fields = PACKET_TYPES[pkt_key]["fields"].copy()
    fmt = PACKET_TYPES[pkt_key]["format"]
    udp_port = PACKET_TYPES[pkt_key]["udp_port"]
    print(f"\nSelected packet type: {PACKET_TYPES[pkt_key]['desc']} ({pkt_key}), UDP port {udp_port}")
    print("Commands: set <field> <value>, send, show, quit")
    while True:
        try:
            cmd = input(f"{pkt_key}> ").strip()
        except (EOFError, KeyboardInterrupt):
            print()
            break

        if cmd == "":
            continue
        elif cmd in ("q", "quit", "exit"):
            break
        elif cmd == "show":
            print_packet(fields)
        elif cmd == "send":
            packet = build_packet(fields, fmt)
            sock.sendto(packet, (UDP_IP, udp_port))
            print(f"Sent {len(packet)} bytes to {UDP_IP}:{udp_port}")
        elif cmd.startswith("set "):
            try:
                _, field, val = cmd.split(maxsplit=2)
                if field not in fields:
                    print(f"Unknown field: {field}")
                    continue
                fields[field] = parse_value(val)
            except Exception as e:
                print(f"Error parsing command: {e}")
        else:
            print("Unknown command")
