#include "amd64proxy.hpp"
#include <cstring>

// Packet structure definitions - use tight packing to match network protocol
#pragma pack(push, 1)

struct clockframe
{
	uint64_t cf_rdi;
	uint64_t cf_rsi;
	uint64_t cf_rdx;
	uint64_t cf_rcx;
	uint64_t cf_r8;
	uint64_t cf_r9;
	uint64_t cf_r10;
	uint64_t cf_r11;
	uint64_t cf_r12;
	uint64_t cf_r13;
	uint64_t cf_r14;
	uint64_t cf_r15;
	uint64_t cf_rbp;
	uint64_t cf_rbx;
	uint64_t cf_rax;
	uint64_t cf_gs;
	uint64_t cf_fs;
	uint64_t cf_es;
	uint64_t cf_ds;
	uint64_t cf_trapno;	/* trap type (always T_PROTFLT for clock interrupts, but unused here) */
	uint64_t cf_err;	/* error code (0 for clock interrupts) */
	uint64_t cf_rip;	/* instruction pointer at interrupt */
	uint64_t cf_cs;		/* code segment at interrupt */
	uint64_t cf_rflags;	/* flags register at interrupt */
	uint64_t cf_rsp;	/* stack pointer at interrupt */
	uint64_t cf_ss;		/* stack segment at interrupt */
};

/* NetBSD x64 Panel structure */
struct netbsdx64_panel_state
{
    struct clockframe ps_frame;        /* panel switches - NetBSD clockframe structure */
};

/* NetBSD x64 Panel packet structure */
struct netbsdx64_panel_packet
{
    struct panel_packet_header header;
    struct netbsdx64_panel_state panel_state;
};

#pragma pack(pop)

AMD64Proxy::AMD64Proxy(unsigned short port)
    : ProxyBase(port)
{
    // Debug: Print structure sizes to verify pragma pack worked
    log_info("Structure sizes: header=%zu bytes, panel_state=%zu bytes, total_packet=%zu bytes",
             sizeof(panel_packet_header), sizeof(netbsdx64_panel_state), sizeof(netbsdx64_panel_packet));
}

std::string AMD64Proxy::udp_packet_to_json(std::span<const char> data)
{
    // Parse the NetBSD x64 panel state
    netbsdx64_panel_state panel_state;
    if (!get_panel_state(data, panel_state))
        return "";

    auto& ps_frame = panel_state.ps_frame;

    crow::json::wvalue json;
    json["rax"]    = to_hex(ps_frame.cf_rax);
    json["rbx"]    = to_hex(ps_frame.cf_rbx);
    json["rcx"]    = to_hex(ps_frame.cf_rcx);
    json["rdx"]    = to_hex(ps_frame.cf_rdx);
    json["rdi"]    = to_hex(ps_frame.cf_rdi);
    json["rsi"]    = to_hex(ps_frame.cf_rsi);
    json["rbp"]    = to_hex(ps_frame.cf_rbp);
    json["rsp"]    = to_hex(ps_frame.cf_rsp);
    json["rip"]    = to_hex(ps_frame.cf_rip);
    json["rflags"] = to_hex(ps_frame.cf_rflags);

    return json.dump();
}
