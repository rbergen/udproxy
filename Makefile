# Simple Makefile for phproxy (UDP→WebSocket Proxy, C++17, header-only deps)

CXX ?= c++
CXXFLAGS = -std=c++17 -Wall -O2 \
  -I. \
  -Iwebsocketpp \
  -Iasio

LDFLAGS = -lpthread

SOURCES = main.cpp pdproxy.cpp
HEADERS = pdproxy.hpp json.hpp
TARGET = phproxy

OBJS = main.o pdproxy.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LDFLAGS) -o $(TARGET)

main.o: main.cpp pdproxy.hpp
pdproxy.o: pdproxy.cpp pdproxy.hpp

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
