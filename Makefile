smart_bath: src/server.cpp
	g++ $< -o bin/$@ -lpistache -lcrypto -lssl -lpthread