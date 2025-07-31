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

# macOS Homebrew paths
BREW_PREFIX := $(shell if [ "$(UNAME_S)" = "Darwin" ] && command -v brew >/dev/null 2>&1; then brew --prefix; fi)
ifneq ($(BREW_PREFIX),)
    CXXFLAGS += -I$(BREW_PREFIX)/include/
    LDFLAGS += -L$(BREW_PREFIX)/lib/
endif

# NetBSD pkgsrc paths  
ifeq ($(UNAME_S),NetBSD)
    ifneq ($(wildcard /usr/pkg/include),)
        CXXFLAGS += -I/usr/pkg/include
        LDFLAGS += -L/usr/pkg/lib
    endif
endif

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
