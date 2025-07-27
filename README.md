# udproxy

```text
   __  ______  ____
  / / / / __ \/ __ \_________  _  ____  __
 / / / / / / / /_/ / ___/ __ \| |/_/ / / /
/ /_/ / /_/ / ____/ /  / /_/ />  </ /_/ /
\____/_____/_/   /_/   \____/_/|_|\__, /
                                 /____/
```

## Overview

**udproxy** is a modular proxy and monitoring tool for different types of CPUs.
It listens for UDP packets containing CPU state, translates them to JSON, and broadcasts them to connected WebSocket clients. The project includes a lightweight web server for serving dashboards and client pages, allowing real-time visualization and interaction via modern browsers.

Multiple proxy modules can run in parallel, each on its own port, and all share a single web server for static content.

## Supported CPUs and Port Numbers

Currently, the proxy includes modules for the following CPUs. The default UDP and WebSocket port number for each module are as indicated:

- PDP-11/83, running at port 4000
- AMD64, running at port 4001

The built-in webserver runs at port 4080.

## Getting the Source Code (Read This!)

Clone the repository *and its dependencies*:

```bash
git clone --recurse-submodules https://github.com/PlummersSoftwareLLC/udproxy.git
```

This will fetch the main source and the embedded `IXWebSocket` and `fmt` libraries.
For pulling in the dependencies, the `--recurse-submodules` flag is critical. If you forgot to add that, you can clone and initialize the submodules after cloning. For this, issue the following command while in the cloned udproxy directory:

```bash
git submodule update --init --recursive
```

`fmt` is used because `std::format` is not available in GCC 11, which is the version in one of the contributor's Linux distribution.

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

   ```text
   http://localhost:4080/
   ```

   You'll see the dashboard and links to available proxy modules.

## Directory Structure

- `main.cpp` — Application entry point
- `proxybase.hpp/cpp` — Abstract base class for proxies
- `pdproxy.hpp/cpp` — PDP-11 proxy implementation
- `amd64proxy.hpp/cpp` — AMD64 proxy implementation
- `webserver.hpp/cpp` — Lightweight HTTP server
- `wwwroot/` — Static web content (dashboard, client pages)
- `IXWebSocket/` — WebSocket library (as a submodule)
- `fmt/` — formatting library (as a submodule)
- A number of other header files provide supporting functions

## Credits

- The PDP-11 virtual panel that is displayed in the browser, is based on the [Javascript PDP 11/70 Emulator](https://github.com/paulnank/pdp11-js) written by Paul Nankervis.
- WebSocket support is provided through [IXWebSocket](https://github.com/machinezone/IXWebSocket).
- The HTTP serving is implemented using [cpp-httplib](https://github.com/yhirose/cpp-httplib).
