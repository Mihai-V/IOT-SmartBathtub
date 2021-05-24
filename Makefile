.PHONY: all build clean run

CXXFLAGS += -std=c++17
LDFLAGS += -lpistache -lcrypto -lssl -lpthread -lpaho-mqttpp3 -lpaho-mqtt3a

all: build run

build: smart_bath

clean:
	-rm smarteeth

run:
	bin/smart_bath

smart_bath: src/server.cpp
	g++ $< -o bin/$@ $(CXXFLAGS) $(LDFLAGS)