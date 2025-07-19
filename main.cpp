#include "pdproxy.hpp"
#include "json.hpp"
#include <iostream>
#include <vector>
#include <memory>
#include <csignal>
#include <thread>

std::vector<std::unique_ptr<ProxyBase>> proxies;

void signal_handler(int)
{
    for (auto& proxy : proxies)
        proxy->stop();
}

int main()
{
    std::signal(SIGINT, signal_handler);

    proxies.emplace_back(std::make_unique<PDProxy>(4000, 4080, "pdproxy"));
    // Example: add more proxies with different ports if needed
    // proxies.emplace_back(std::make_unique<PDProxy>(5000, 5080, "./otherdir"));

    // Run all proxies in parallel
    std::vector<std::thread> threads;

    for (auto& proxy : proxies)
        threads.emplace_back([&proxy]() { proxy->run(); });

    for (auto& t : threads)
    {
        if (t.joinable())
            t.join();
    }

    return 0;
}
