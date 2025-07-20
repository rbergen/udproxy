#include "pdproxy.hpp"
#include "webserver.hpp"
#include "json.hpp"
#include <iostream>
#include <vector>
#include <memory>
#include <csignal>
#include <thread>

#define WEBSERVER_PORT 4080
#define CONTENT_DIR "wwwroot"
#define PDPROXY_NAME "pdproxy"
#define PDPROXY_PORT 4000

static std::vector<std::unique_ptr<ProxyBase>> proxies;
static std::unique_ptr<WebServer> webserver;

void signal_handler(int)
{
    for (auto& proxy : proxies)
        proxy->stop();
    if (webserver)
        webserver->stop();
}

void add_proxy(const std::string& name, unsigned short port)
{
    webserver->add_proxy_port(name, port);
    auto proxy = std::make_unique<PDProxy>(port);
    proxies.push_back(std::move(proxy));
}

int main()
{
    std::signal(SIGINT, signal_handler);

    // Create shared webserver
    webserver = std::make_unique<WebServer>(WEBSERVER_PORT, CONTENT_DIR);

    add_proxy(PDPROXY_NAME, PDPROXY_PORT);
    // Example: add more proxies with different ports if needed
    // add_proxy("proxy2", 5000);

    auto webserver_thread = std::thread([]() { webserver->run(); });

    // Run all proxies in parallel
    std::vector<std::thread> threads;
    for (auto& proxy : proxies)
        threads.emplace_back([&proxy]() { proxy->run(); });

    for (auto& t : threads)
    {
        if (t.joinable())
            t.join();
    }

    if (webserver_thread.joinable())
        webserver_thread.join();

    return 0;
}
