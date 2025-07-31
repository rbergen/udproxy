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
# Use a more compatible approach for NetBSD make
UNAME_S != uname -s

# Platform-specific include and library paths
# Simplified approach for better NetBSD make compatibility
NETBSD_INCLUDES != if [ "$(UNAME_S)" = "NetBSD" ] && [ -d "/usr/pkg/include" ]; then echo "-I/usr/pkg/include"; fi
NETBSD_LIBS != if [ "$(UNAME_S)" = "NetBSD" ] && [ -d "/usr/pkg/lib" ]; then echo "-L/usr/pkg/lib"; fi

DARWIN_INCLUDES != if [ "$(UNAME_S)" = "Darwin" ] && command -v brew >/dev/null 2>&1; then echo "-I$$(brew --prefix)/include/"; fi
DARWIN_LIBS != if [ "$(UNAME_S)" = "Darwin" ] && command -v brew >/dev/null 2>&1; then echo "-L$$(brew --prefix)/lib/"; fi

CXXFLAGS += $(NETBSD_INCLUDES) $(DARWIN_INCLUDES)
LDFLAGS += $(NETBSD_LIBS) $(DARWIN_LIBS)

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
	@echo "NETBSD_INCLUDES: $(NETBSD_INCLUDES)"
	@echo "NETBSD_LIBS: $(NETBSD_LIBS)"
	@echo "DARWIN_INCLUDES: $(DARWIN_INCLUDES)"
	@echo "DARWIN_LIBS: $(DARWIN_LIBS)"
	@echo "Final CXXFLAGS: $(CXXFLAGS)"
	@echo "Final LDFLAGS: $(LDFLAGS)"
	@echo "Directory checks:"
	@echo "  /usr/pkg/include exists: $$(test -d /usr/pkg/include && echo YES || echo NO)"
	@echo "  /usr/pkg/lib exists: $$(test -d /usr/pkg/lib && echo YES || echo NO)"

-include $(DEPS)
