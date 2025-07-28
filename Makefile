# VPanel Root Makefile
# Builds both socket and proxy components

.PHONY: all socket proxy clean clean-socket clean-proxy help

# Default target builds everything
all: socket proxy

# Build socket component (client and server)
socket:
	@echo "Building socket component..."
	@cd socket && $(MAKE) all

# Build proxy component (UDP to WebSocket proxy)
proxy:
	@echo "Building proxy component..."
	@cd proxy && $(MAKE) all

# Clean everything
clean: clean-socket clean-proxy

# Clean socket component
clean-socket:
	@echo "Cleaning socket component..."
	@cd socket && $(MAKE) clean

# Clean proxy component  
clean-proxy:
	@echo "Cleaning proxy component..."
	@cd proxy && $(MAKE) clean

# Show help
help:
	@echo "VPanel Build System"
	@echo "=================="
	@echo ""
	@echo "Available targets:"
	@echo "  all          - Build both socket and proxy components (default)"
	@echo "  socket       - Build socket component (client and server)"
	@echo "  proxy        - Build proxy component (UDP to WebSocket proxy)"
	@echo "  clean        - Clean all build artifacts"
	@echo "  clean-socket - Clean only socket build artifacts"
	@echo "  clean-proxy  - Clean only proxy build artifacts"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Components:"
	@echo "  socket/      - Platform-specific clients and UDP server"
	@echo "  proxy/       - UDP to WebSocket proxy with web interface"
