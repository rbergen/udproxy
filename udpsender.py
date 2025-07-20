#!/usr/bin/env python3
# udpsender.py - A simple UDP sender for PDP-11 panel packets
# This script allows you to set fields of a PDP-11 packet and send it over UDP

import socket
import struct
import readline  # allows up/down arrow history

# Defaults
UDP_IP = "127.0.0.1"
UDP_PORT = 4000

# Initial field values
packet_fields = {
    "pp_byte_count": 16,
    "pp_byte_flags": 1,
    "ps_address": 0x15555,
    "ps_data": 0xAAAA,
    "ps_psw": 0x0200,
    "ps_mser": 0x0001,
    "ps_cpu_err": 0x0000,
    "ps_mmr0": 0x7000,
    "ps_mmr3": 0x0003,
}

def build_packet(fields):
    """Pack the current field values into a binary packet"""
    return struct.pack(
        "<HI I 6H",
        fields["pp_byte_count"],
        fields["pp_byte_flags"],
        fields["ps_address"],
        fields["ps_data"],
        fields["ps_psw"],
        fields["ps_mser"],
        fields["ps_cpu_err"],
        fields["ps_mmr0"],
        fields["ps_mmr3"]
    )

def print_packet(fields):
    print("Current packet field values:")
    for k, v in fields.items():
        print(f"  {k} = 0x{v:04X}")

def parse_value(val):
    """Parse a value like 0x1234 or 4660"""
    return int(val, 0)

# Create UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

print("UDP PDP Panel Packet Sender")
print(f"Target: {UDP_IP}:{UDP_PORT}")
print("Commands: set <field> <value>, send, show, quit")

while True:
    try:
        cmd = input("> ").strip()
    except (EOFError, KeyboardInterrupt):
        break

    if cmd == "":
        continue
    elif cmd in ("q", "quit", "exit"):
        break
    elif cmd == "show":
        print_packet(packet_fields)
    elif cmd == "send":
        packet = build_packet(packet_fields)
        sock.sendto(packet, (UDP_IP, UDP_PORT))
        print(f"Sent {len(packet)} bytes")
    elif cmd.startswith("set "):
        try:
            _, field, val = cmd.split(maxsplit=2)
            if field not in packet_fields:
                print(f"Unknown field: {field}")
                continue
            packet_fields[field] = parse_value(val)
        except Exception as e:
            print(f"Error parsing command: {e}")
    else:
        print("Unknown command")
