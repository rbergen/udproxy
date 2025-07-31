#include "webserver.hpp"

WebServer::WebServer(unsigned short port, std::string content_dir)
    : port(port)
    , content_dir(std::move(content_dir))
{
    server.loglevel(crow::LogLevel::Warning); // Set log level to Info
}

WebServer::~WebServer()
{
    server.stop();
}

void WebServer::run()
{
    // Config endpoint
    CROW_ROUTE(server, "/config.json")
    ([&]
    {
        // Build the JSON body
        crow::json::wvalue json;
        auto& ports_json = json["proxy_ports"];
        for (const auto& [name, port] : proxy_ports)
            ports_json[name] = port;

        crow::response res{json.dump()};
        res.set_header("Content-Type", "application/json");
        return res;
    });

    // Root ‑ index.html
    CROW_ROUTE(server, "/")
    ([&]
    {
        crow::response res;
        res.set_static_file_info(content_dir + "/index.html");
        return res;
    });

    // Everything else from static/
    CROW_ROUTE(server, "/<path>")
    ([&](const std::string& path)
    {
        crow::response res;
        res.set_static_file_info(content_dir + "/" + path);
        return res;
    });

    auto server_future = server.port(port).multithreaded().run_async();

    if (server.wait_for_server_start() != std::cv_status::no_timeout)
    {
        log_error("Failed to start HTTP server on port %u", port);
        return;
    }

    log_info("HTTP server serving directory '%s' on port %u", content_dir.c_str(), port);

    server_future.wait();
    log_info("HTTP server stopped");
    server_future.get();
}

void WebServer::stop()
{
    server.stop();
}

void WebServer::add_proxy_port(const std::string& proxy_name, unsigned short port)
{
    proxy_ports[proxy_name] = port;
}
