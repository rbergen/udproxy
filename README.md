# udproxy
   __  ______  ____                       
  / / / / __ \/ __ \_________  _  ____  __
 / / / / / / / /_/ / ___/ __ \| |/_/ / / /
/ /_/ / /_/ / ____/ /  / /_/ />  </ /_/ / 
\____/_____/_/   /_/   \____/_/|_|\__, /  
                                 /____/   

## Overview

**udproxy** is a modular proxy and monitoring tool for PDP-11 emulation and hardware projects. It listens for UDP packets containing PDP-11 state, translates them to JSON, and broadcasts them to connected WebSocket clients. The project includes a lightweight web server for serving dashboards and client pages, allowing real-time visualization and interaction via modern browsers.

Multiple proxy modules can run in parallel, each on its own port, and all share a single web server for static content. The project uses [IXWebSocket](https://github.com/machinezone/IXWebSocket) for WebSocket support and [cpp-httplib](https://github.com/yhirose/cpp-httplib) for HTTP serving.

## Getting the Source Code

Clone the repository and its dependencies:

```bash
git clone --recurse-submodules https://github.com/PlummersSoftwareLLC/udproxy.git
```

This will fetch the main source and the embedded `IXWebSocket` library.

## Build Instructions

### Prerequisites

- C++20 compiler (e.g., `g++`, `clang++`)
- CMake (for building IXWebSocket)
- zlib development headers (`sudo apt-get install zlib1g-dev` on Ubuntu)
- pthreads (usually included by default)

### Build Steps (pdproxy and IXWebSocket dependency)

   ```bash
   make
   ```

   This will first build the IXWebSocket library and then the `udproxy` executable in the project root.

## Running

1. Start the proxy:

   ```bash
   ./udproxy
   ```

2. Open your browser and navigate to:

   ```
   http://localhost:4080/
   ```

   You'll see the dashboard and links to available proxy modules.

## Directory Structure

- `main.cpp` — Application entry point
- `proxybase.hpp/cpp` — Abstract base class for proxies
- `pdproxy.hpp/cpp` — PDP-11 proxy implementation
- `webserver.hpp/cpp` — Lightweight HTTP server
- `wwwroot/` — Static web content (dashboard, client pages)
- `IXWebSocket/` — WebSocket library (as a submodule)
