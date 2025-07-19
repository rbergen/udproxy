# Makefile for pdproxy (UDP→WebSocket Proxy, C++20)

CXX ?= c++

IXWS_DIR = ./IXWebSocket
IXWS_BUILD_DIR = $(IXWS_DIR)/build

UNAME_S := $(shell uname -s)

CXXFLAGS = -std=c++20 -Wall -O2 -MMD -MP -I. -I$(IXWS_DIR)
LDFLAGS = -L$(IXWS_BUILD_DIR) -lixwebsocket -lpthread -lz

DEP_DIR = dep
SOURCES = main.cpp pdproxy.cpp proxybase.cpp webserver.cpp
TARGET = udproxy

OBJS = $(SOURCES:.cpp=.o)
DEPS = $(addprefix $(DEP_DIR)/, $(notdir $(OBJS:.o=.d)))

ifeq ($(UNAME_S), Darwin)
LIBIXWEBSOCKET = $(IXWS_BUILD_DIR)/libixwebsocket.a
else
LIBIXWEBSOCKET = $(IXWS_BUILD_DIR)/libixwebsocket.a
endif

all: $(TARGET)

$(LIBIXWEBSOCKET):
ifeq ($(UNAME_S), Darwin)
	mkdir -p $(IXWS_BUILD_DIR) && cd $(IXWS_BUILD_DIR) && cmake -DUSE_ZLIB=1 .. && make -j
else
	mkdir -p $(IXWS_BUILD_DIR) && cd $(IXWS_BUILD_DIR) && cmake -DUSE_ZLIB=1 .. && make -j
endif

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
