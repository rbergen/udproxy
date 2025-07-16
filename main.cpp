#include "pdproxy.hpp"
#include "json.hpp"
#include <iostream>

int main()
{
    unsigned short udp_port = 4000;
    unsigned short ws_port = 9002;
    PDProxy proxy(udp_port, ws_port);
    proxy.run();
    return 0;
}
