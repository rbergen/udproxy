# Simple Makefile for pdproxy (UDP→WebSocket Proxy, C++20, header-only deps)

CXX ?= c++
CXXFLAGS = -std=c++20 -Wall -O2 -I. -I./IXWebSocket

LDFLAGS = -lpthread -lz

IXWS_SRC = \
    IXWebSocket/ixwebsocket/IXWebSocketServer.cpp \
    IXWebSocket/ixwebsocket/IXWebSocket.cpp \
    IXWebSocket/ixwebsocket/IXNetSystem.cpp \
    IXWebSocket/ixwebsocket/IXSocket.cpp \
    IXWebSocket/ixwebsocket/IXDNSLookup.cpp \
    IXWebSocket/ixwebsocket/IXWebSocketPerMessageDeflate.cpp \
    IXWebSocket/ixwebsocket/IXWebSocketHandshake.cpp \
    IXWebSocket/ixwebsocket/IXWebSocketHttpHeaders.cpp \
    IXWebSocket/ixwebsocket/IXWebSocketTransport.cpp \
    IXWebSocket/ixwebsocket/IXWebSocketSendState.cpp

SOURCES = main.cpp pdproxy.cpp $(IXWS_SRC)
TARGET = pdproxy

OBJS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LDFLAGS) -o $(TARGET)

main.o: main.cpp pdproxy.hpp
pdproxy.o: pdproxy.cpp pdproxy.hpp

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
