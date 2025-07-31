# Makefile for VPanel proxy (UDP→WebSocket Proxy, C++20)

CXX ?= c++

CXXFLAGS = -std=c++20 -Wall -O2 -MMD -MP -I.
LDFLAGS = -lpthread -lz

DEP_DIR = dep
SOURCES = main.cpp amd64proxy.cpp pdproxy.cpp netbsdvaxproxy.cpp proxybase.cpp webserver.cpp
TARGET = udproxy

OBJS = $(SOURCES:.cpp=.o)
DEPS = $(addprefix $(DEP_DIR)/, $(notdir $(OBJS:.o=.d)))

# Add platform-specific paths for dependencies
UNAME_S := $(shell uname -s)

# Platform-specific include and library paths
PLATFORM_CXXFLAGS := $(shell \
	if [ "$(UNAME_S)" = "Darwin" ] && command -v brew >/dev/null 2>&1; then \
		echo "-I$$(brew --prefix)/include/"; \
	elif [ "$(UNAME_S)" = "NetBSD" ] && [ -d "/usr/pkg/include" ]; then \
		echo "-I/usr/pkg/include"; \
	fi)

PLATFORM_LDFLAGS := $(shell \
	if [ "$(UNAME_S)" = "Darwin" ] && command -v brew >/dev/null 2>&1; then \
		echo "-L$$(brew --prefix)/lib/"; \
	elif [ "$(UNAME_S)" = "NetBSD" ] && [ -d "/usr/pkg/lib" ]; then \
		echo "-L/usr/pkg/lib"; \
	fi)

CXXFLAGS += $(PLATFORM_CXXFLAGS)
LDFLAGS += $(PLATFORM_LDFLAGS)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)

$(DEP_DIR):
	@mkdir -p $(DEP_DIR)

%.o: %.cpp | $(DEP_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MF $(DEP_DIR)/$(@:.o=.d)

.PHONY: all clean debug

debug:
	@echo "UNAME_S: $(UNAME_S)"
	@echo "PLATFORM_CXXFLAGS: $(PLATFORM_CXXFLAGS)"
	@echo "PLATFORM_LDFLAGS: $(PLATFORM_LDFLAGS)"
	@echo "Final CXXFLAGS: $(CXXFLAGS)"
	@echo "Final LDFLAGS: $(LDFLAGS)"

-include $(DEPS)
