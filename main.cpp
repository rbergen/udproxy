#include "pdproxy.hpp"
#include "webserver.hpp"
#include "json.hpp"
#include <iostream>
#include <vector>
#include <memory>
#include <csignal>
#include <thread>

static std::vector<std::unique_ptr<ProxyBase>> proxies;
static std::unique_ptr<WebServer> webserver;

void signal_handler(int)
{
    for (auto& proxy : proxies)
        proxy->stop();
    if (webserver)
        webserver->stop();
}

int main()
{
    std::signal(SIGINT, signal_handler);

    proxies.emplace_back(std::make_unique<PDProxy>(4000));
    // Example: add more proxies with different ports if needed
    // proxies.emplace_back(std::make_unique<PDProxy>(5000);

    // Start shared webserver
    webserver = std::make_unique<WebServer>(4080, "wwwroot");
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
