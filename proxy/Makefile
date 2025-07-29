# Makefile for VPanel proxy (UDP→WebSocket Proxy, C++20)

CXX ?= c++

CXXFLAGS = -std=c++20 -Wall -O2 -MMD -MP -I.
LDFLAGS = -lpthread -lz

DEP_DIR = dep
SOURCES = main.cpp amd64proxy.cpp pdproxy.cpp netbsdvaxproxy.cpp proxybase.cpp webserver.cpp
TARGET = udproxy

OBJS = $(SOURCES:.cpp=.o)
DEPS = $(addprefix $(DEP_DIR)/, $(notdir $(OBJS:.o=.d)))

# Add brew paths for macOS
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S), Darwin)
    CXXFLAGS += -I$(shell brew --prefix)/include/
    LDFLAGS += -L$(shell brew --prefix)/lib/
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
