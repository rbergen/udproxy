# Simple Makefile for phproxy (UDP→WebSocket Proxy, C++17, header-only deps)

CXX ?= c++
CXXFLAGS = -std=c++17 -Wall -O2 \
  -I. \
  -Iwebsocketpp \
  -Iasio

LDFLAGS = -lpthread

SOURCES = main.cpp phproxy.cpp
HEADERS = phproxy.hpp json.hpp
TARGET = phproxy

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SOURCES) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean
