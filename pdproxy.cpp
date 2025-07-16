#include "pdproxy.hpp"
#include <set>
#include <vector>
#include <cstdint>

using asio::ip::udp;
typedef websocketpp::server<websocketpp::config::asio> ws_server;
using connection_hdl = websocketpp::connection_hdl;

struct PDProxy::Impl
{
    asio::io_context io_ctx;
    udp::socket socket;
    udp::endpoint remote_endpoint;
    ws_server ws_server;
    std::set<connection_hdl, std::owner_less<connection_hdl>> ws_clients;
    std::vector<char> buffer;

    unsigned short udp_port;
    unsigned short ws_port;
    PDProxy* parent;

    Impl(unsigned short udp, unsigned short ws, PDProxy* p)
        : socket(io_ctx), udp_port(udp), ws_port(ws), parent(p) {}

    void setup_ws_server()
    {
        ws_server.set_reuse_addr(true);
        ws_server.init_asio(&io_ctx);
        ws_server.set_open_handler([this](connection_hdl hdl) { ws_clients.insert(hdl); });
        ws_server.set_close_handler([this](connection_hdl hdl) { ws_clients.erase(hdl); });
        ws_server.listen(ws_port);
        ws_server.start_accept();
    }

    void setup_udp() {
        socket.open(udp::v4());
        socket.bind(udp::endpoint(udp::v4(), udp_port));
        start_receive();
    }

    void start_receive()
    {
        socket.async_receive_from(
            asio::buffer(buffer), remote_endpoint,
            [this](std::error_code ec, std::size_t bytes_recvd)
            {
                if (!ec && bytes_recvd > 0)
                {
                    std::string payload = parent->udp_packet_to_json(std::span<const char>(buffer.data(), bytes_recvd));
                    if (!payload.empty() && !ws_clients.empty())
                        forward_to_ws_clients(payload);
                }
                start_receive();
            }
        );
    }

    void forward_to_ws_clients(const std::string& data)
    {
        for (auto it = ws_clients.begin(); it != ws_clients.end();)
        {
            websocketpp::lib::error_code ec;
            ws_server.send(*it, data, websocketpp::frame::opcode::text, ec);
            if (ec)
                it = ws_clients.erase(it);
            else
                ++it;
        }
    }
};

PDProxy::PDProxy(unsigned short udp_port, unsigned short ws_port)
    : pImpl(std::make_unique<Impl>(udp_port, ws_port, this))
{
    pImpl->setup_ws_server();
    pImpl->setup_udp();
}

PDProxy::~PDProxy() = default;

void PDProxy::run()
{
    pImpl->io_ctx.run();
}

std::string PDProxy::udp_packet_to_json(std::span<const char> data)
{
    if (data.size() != 16) return "{}";

    uint32_t address;
    uint16_t ps_data, ps_psw, ps_mser, ps_cpu_err, ps_mmr0, ps_mmr3;

    memcpy(&address,  &data[0],  4);
    memcpy(&ps_data,  &data[4],  2);
    memcpy(&ps_psw,   &data[6],  2);
    memcpy(&ps_mser,  &data[8],  2);
    memcpy(&ps_cpu_err, &data[10], 2);
    memcpy(&ps_mmr0,  &data[12], 2);
    memcpy(&ps_mmr3,  &data[14], 2);

    int mode = (ps_psw >> 14) & 0x3;
    bool data_on = (ps_psw & (1 << 13));

    nlohmann::json json;
    json["ParityError"]  = (ps_mser & 0x7380) ? 1 : 0;
    json["AddressError"] = (ps_cpu_err & 0xfc00) ? 1 : 0;
    json["Run"]          = 1;
    json["Pause"]        = 0;
    json["Master"]       = 1;
    json["UserMode"]     = (mode == 3);
    json["SuperMode"]    = (mode == 1);
    json["KernelMode"]   = (mode == 0);
    json["DataOn"]       = data_on ? 1 : 0;
    json["Addr16"]       = !(ps_mmr3 & ((1 << 4) | (1 << 5)));
    json["Addr18"]       = (ps_mmr3 & (1 << 5)) && !(ps_mmr3 & (1 << 4));
    json["Addr22"]       = (ps_mmr3 & (1 << 4));
    json["UserD"]        = (mode == 3) && data_on;
    json["UserI"]        = (mode == 3) && !data_on;
    json["SuperD"]       = (mode == 1) && data_on;
    json["SuperI"]       = (mode == 1) && !data_on;
    json["KernelD"]      = (mode == 0) && data_on;
    json["KernelI"]      = (mode == 0) && !data_on;
    json["ConsPhy"]      = 0;
    json["ProgPhy"]      = !(ps_mmr0 & 1);
    json["ParityHigh"]   = (ps_mser & (1 << 9)) ? 1 : 0;
    json["ParityLow"]    = (ps_mser & (1 << 8)) ? 1 : 0;
    
    return json.dump();
}
