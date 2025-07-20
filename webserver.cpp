#include "webserver.hpp"
#include "json.hpp"

WebServer::WebServer(unsigned short port, std::string content_dir)
    : port(port)
    , content_dir(std::move(content_dir))
    , server(nullptr)
    , running(false)
{
}

WebServer::~WebServer()
{
    stop();
    if (server_thread.joinable())
        server_thread.join();
}

const char* WebServer::module_name() const {
    return "WebServer";
}

void WebServer::run()
{
    running = true;

    server = std::make_unique<httplib::Server>();
    server->set_mount_point("/", content_dir.c_str());

     // Add config endpoint
    server->Get("/config.json", [this](const httplib::Request&, httplib::Response& res) {
        nlohmann::json config;
        config["proxyPorts"] = proxy_ports; // will be a JSON object mapping names to ports
        res.set_content(config.dump(), "application/json");
    });

    log_info("HTTP server serving directory '%s' on port %u", content_dir.c_str(), port);
    server->listen("0.0.0.0", port);
    log_info("HTTP server stopped");

    running = false;
    server.reset();
}

void WebServer::stop()
{
    if (server)
        server->stop();
    running = false;
}

void WebServer::add_proxy_port(const std::string& proxy_name, unsigned short port) {
    proxy_ports[proxy_name] = port;
}
