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
# Pure shell approach compatible with both NetBSD make and GNU make

# Detect platform and set paths in one go
PLATFORM_PATHS != \
	OS=$$(uname -s); \
	if [ "$$OS" = "NetBSD" ] && [ -d "/usr/pkg/include" ]; then \
		echo "-I/usr/pkg/include|-L/usr/pkg/lib"; \
	elif [ "$$OS" = "Darwin" ] && command -v brew >/dev/null 2>&1; then \
		BREW_PREFIX=$$(brew --prefix); \
		echo "-I$$BREW_PREFIX/include/|-L$$BREW_PREFIX/lib/"; \
	fi

# Extract include and library paths (fallback for GNU make)
PLATFORM_PATHS_FALLBACK := $(shell \
	OS=$$(uname -s); \
	if [ "$$OS" = "NetBSD" ] && [ -d "/usr/pkg/include" ]; then \
		echo "-I/usr/pkg/include|-L/usr/pkg/lib"; \
	elif [ "$$OS" = "Darwin" ] && command -v brew >/dev/null 2>&1; then \
		BREW_PREFIX=$$(brew --prefix); \
		echo "-I$$BREW_PREFIX/include/|-L$$BREW_PREFIX/lib/"; \
	fi)

# Use NetBSD result if available, otherwise use GNU make fallback
FINAL_PATHS := $(if $(PLATFORM_PATHS),$(PLATFORM_PATHS),$(PLATFORM_PATHS_FALLBACK))

# Split the paths
PLATFORM_INCLUDES := $(shell echo "$(FINAL_PATHS)" | cut -d'|' -f1)
PLATFORM_LIBS := $(shell echo "$(FINAL_PATHS)" | cut -d'|' -f2)

CXXFLAGS += $(PLATFORM_INCLUDES)
LDFLAGS += $(PLATFORM_LIBS)

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
	@echo "Platform detection:"
	@echo "  OS: $$(uname -s)"
	@echo "PLATFORM_PATHS: $(PLATFORM_PATHS)"
	@echo "PLATFORM_PATHS_FALLBACK: $(PLATFORM_PATHS_FALLBACK)"
	@echo "FINAL_PATHS: $(FINAL_PATHS)"
	@echo "PLATFORM_INCLUDES: $(PLATFORM_INCLUDES)"
	@echo "PLATFORM_LIBS: $(PLATFORM_LIBS)"
	@echo "Final CXXFLAGS: $(CXXFLAGS)"
	@echo "Final LDFLAGS: $(LDFLAGS)"
	@echo "Directory checks:"
	@echo "  /usr/pkg/include exists: $$(test -d /usr/pkg/include && echo YES || echo NO)"
	@echo "  /usr/pkg/lib exists: $$(test -d /usr/pkg/lib && echo YES || echo NO)"

-include $(DEPS)
