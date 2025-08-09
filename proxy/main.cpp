#include "pdproxy.hpp"
#include "amd64proxy.hpp"
#include "netbsdvaxproxy.hpp"
#include "webserver.hpp"
#include <concepts>
#include <iostream>
#include <vector>
#include <memory>
#include <csignal>
#include <thread>
#include <atomic>

#define WEBSERVER_PORT 4080
#define CONTENT_DIR "wwwroot"
#define PDPROXY_NAME "pdproxy"
#define PDPROXY_PORT 4000
#define AMD64PROXY_NAME "amd64proxy"
#define AMD64PROXY_PORT 4001
#define NETBSDVAXPROXY_NAME "netbsdvax"
#define NETBSDVAXPROXY_PORT 4002

static std::vector<std::unique_ptr<ProxyBase>> proxies;
static std::unique_ptr<WebServer> webserver;
static std::atomic<bool> g_shutdown_requested{false};

void signal_handler(int)
{
    // Async-signal-safe: only set a flag
    g_shutdown_requested.store(true);
}

template<typename T>
    requires std::derived_from<T, ProxyBase>
void add_proxy(const std::string& name, unsigned short port)
{
    webserver->add_proxy_port(name, port);
    auto proxy = std::make_unique<T>(port);
    proxies.push_back(std::move(proxy));
}

int main()
{
    std::signal(SIGINT, signal_handler);

    // Create shared webserver
    webserver = std::make_unique<WebServer>(WEBSERVER_PORT, CONTENT_DIR);

    add_proxy<PDProxy>(PDPROXY_NAME, PDPROXY_PORT);
    add_proxy<AMD64Proxy>(AMD64PROXY_NAME, AMD64PROXY_PORT);
    add_proxy<NetBSDVAXProxy>(NETBSDVAXPROXY_NAME, NETBSDVAXPROXY_PORT);
    // Example: add more proxies with different ports if needed
    // add_proxy("proxy2", 5000);

    auto webserver_thread = std::thread([]() { webserver->run(); });

    // Run all proxies in parallel
    std::vector<std::thread> threads;
    for (auto& proxy : proxies)
        threads.emplace_back([&proxy]() { proxy->run(); });

    // Wait for shutdown request
    while (!g_shutdown_requested.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Graceful shutdown initiated by signal
    for (auto& proxy : proxies)
        proxy->stop();
    if (webserver)
        webserver->stop();

    for (auto& t : threads)
    {
        if (t.joinable())
            t.join();
    }

    if (webserver_thread.joinable())
        webserver_thread.join();

    return 0;
}
