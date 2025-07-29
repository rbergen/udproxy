# VPanel

```text
 _    ______                   __
| |  / / __ \____ _____  ___  / /
| | / / /_/ / __ `/ __ \/ _ \/ /
| |/ / ____/ /_/ / / / /  __/ /
|___/_/    \__,_/_/ /_/\___/_/
```

## Overview

**VPanel** is a project that contains everything that's needed to extract the CPU state from a number of CPU architectures and operating systems, and display it in a matching virtual panel that can be loaded in a modern web browser.

In general terms, it consists of two parts:

- In the [socket subdirectory](./socket/), a number of OS and architecture-specific kernel modules with supporting tools; each set is located in their respective subdirectory of [socket/arch](./socket/arch/). The combination of kernel module and UDP client ("client.c") compose and send UDP packets containing CPU state to servers able to process them.
- In the [proxy subdirectory](./proxy/), a modular proxy and webserver for converting the UDP packets into WebSocket messages, and having them visualized in in-browser virtual panels.

Both parts have their own README in the respective directory.

## Getting the Source Code

Clone the repository

```bash
git clone https://github.com/PlummersSoftwareLLC/vpanel.git
```
