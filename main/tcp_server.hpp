#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class TCPServer {
private:
	uint16_t port;
	int serverSocket = 0;
	std::vector<int> clientSockets;
	int ipAddress;

	static int init(uint16_t port);
	static void makeSocketNonBlocking(int socket);

public:
	TCPServer(uint16_t port);
	~TCPServer();
	void accept();
	int send(int socket, const std::string& data);
	int recieve(int socket, std::string& data, std::size_t bufferSize);
	int getServerSocket();
	void close(int socket);
	const std::vector<int>& getClientSockets();

};
