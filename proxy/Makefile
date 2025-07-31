# Makefile for VPanel proxy (UDP→WebSocket Proxy, C++20)

CXX ?= c++

CXXFLAGS = -std=c++20 -Wall -O2 -MMD -MP -I.
LDFLAGS = -lpthread -lz

DEP_DIR = dep
SOURCES = main.cpp amd64proxy.cpp pdproxy.cpp netbsdvaxproxy.cpp proxybase.cpp webserver.cpp
TARGET = udproxy

OBJS = $(SOURCES:.cpp=.o)
DEPS = $(addprefix $(DEP_DIR)/, $(notdir $(OBJS:.o=.d)))

# Add brew paths for macOS (portable syntax for different make implementations)
UNAME_S := $(shell uname -s)
BREW_PREFIX := $(shell if [ "$(UNAME_S)" = "Darwin" ] && command -v brew >/dev/null 2>&1; then brew --prefix; fi)
CXXFLAGS += $(if $(BREW_PREFIX),-I$(BREW_PREFIX)/include/)
LDFLAGS += $(if $(BREW_PREFIX),-L$(BREW_PREFIX)/lib/)

# Add NetBSD package paths for asio and other dependencies
NETBSD_PKG_PREFIX := $(shell if [ "$(UNAME_S)" = "NetBSD" ] && [ -d "/usr/pkg/include" ]; then echo "/usr/pkg"; fi)
CXXFLAGS += $(if $(NETBSD_PKG_PREFIX),-I$(NETBSD_PKG_PREFIX)/include)
LDFLAGS += $(if $(NETBSD_PKG_PREFIX),-L$(NETBSD_PKG_PREFIX)/lib)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)

$(DEP_DIR):
	@mkdir -p $(DEP_DIR)

%.o: %.cpp | $(DEP_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MF $(DEP_DIR)/$(@:.o=.d)

.PHONY: all clean

-include $(DEPS)
