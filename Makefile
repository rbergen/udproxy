# Makefile for pdproxy (UDP→WebSocket Proxy, C++20)

CXX ?= c++

CXXFLAGS = -std=c++20 -Wall -O2 -MMD -MP -I. -I$(IXWS_DIR)
LDFLAGS = -L$(IXWS_BUILD_DIR) -lixwebsocket -lpthread -lz

IXWS_DIR = ./IXWebSocket
IXWS_BUILD_DIR = $(IXWS_DIR)/build

DEP_DIR = dep
SOURCES = main.cpp pdproxy.cpp proxybase.cpp webserver.cpp
TARGET = udproxy
LIBIXWEBSOCKET = $(IXWS_BUILD_DIR)/libixwebsocket.a

OBJS = $(SOURCES:.cpp=.o)
DEPS = $(addprefix $(DEP_DIR)/, $(notdir $(OBJS:.o=.d)))

all: $(TARGET)

$(LIBIXWEBSOCKET):
	mkdir -p $(IXWS_BUILD_DIR) && cd $(IXWS_BUILD_DIR) && cmake -DUSE_ZLIB=1 .. && make -j

$(TARGET): $(OBJS) $(LIBIXWEBSOCKET)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)

$(DEP_DIR):
	@mkdir -p $(DEP_DIR)

%.o: %.cpp | $(DEP_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MF $(DEP_DIR)/$(@:.o=.d)

.PHONY: all clean

-include $(DEPS)
