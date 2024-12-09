#include <cstddef>
#include <cstdint>
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "lwip/sockets.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <tcp_server.hpp>
#include <algorithm>

static const char *TAG = "TCP_SERVER";

TCPServer::TCPServer(uint16_t port) : port(port) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    while (this->serverSocket <= 0) {
        this->serverSocket = TCPServer::init(this->port);
    }

    makeSocketNonBlocking(this->serverSocket);
}

TCPServer::~TCPServer() {
    for (auto& clientSocket : this->clientSockets)
        close(clientSocket);
    close(this->serverSocket);
}

int TCPServer::init(uint16_t port) {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return -1;
    }

    ESP_LOGI(TAG, "Socket created");

    struct sockaddr_in server_addr;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    int err = bind(serverSocket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        return -2;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", port);

    err = listen(serverSocket, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        return -3;
    }

    ESP_LOGI(TAG, "Socket listening");
    return serverSocket;
}

void TCPServer::accept() {
    struct sockaddr_in client_addr;
    socklen_t client_size = sizeof(client_addr);

    int clientSocket = ::accept(this->serverSocket, (struct sockaddr*)&client_addr, &client_size);

    if (clientSocket >= 0) {
        ESP_LOGI(TAG, "Client acceped on socket %d", clientSocket);
        makeSocketNonBlocking(clientSocket);
        clientSockets.emplace_back(clientSocket);
        return;
    } 
    if (errno == EWOULDBLOCK || errno == EAGAIN) return;

    ESP_LOGE(TAG, "recv failed: errno %d", errno);
}

int TCPServer::send(int socket, const std::string& data) {
    int err = ::send(socket, data.c_str(), data.length() + 1, 0);

    if (err < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;

        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
        this->close(socket);
        return errno;
    }

    return 0;
}

int TCPServer::recieve(int socket, std::string& data, std::size_t bufferSize) {
    char buffer[bufferSize];
    int len = recv(socket, buffer, bufferSize, 0);

    if (len > 0) {
        data.assign(buffer);
        return 1;
    } 

    if (len == 0){
        ESP_LOGI(TAG, "Client on socket %d closed connection", socket);
        this->close(socket);
        return 0;
    }

    if (errno == EWOULDBLOCK || errno == EAGAIN) return 0;

    ESP_LOGE(TAG, "recv failed: errno %d", errno);
    this->close(socket);
    return errno;
}

void TCPServer::makeSocketNonBlocking(int socket) {
    int flags = fcntl(socket, F_GETFL, 0);
    fcntl(socket, F_SETFL, flags | O_NONBLOCK);
}

int TCPServer::getServerSocket() {
    return this->serverSocket;
}

const std::vector<int>& TCPServer::getClientSockets() {
    return this->clientSockets;
}

void TCPServer::close(int socket) {
        ::close(socket);
        this->clientSockets.erase(std::remove(this->clientSockets.begin(), this->clientSockets.end(), socket), this->clientSockets.end());
        ESP_LOGI(TAG, "Closing client on socket %d", socket);
}
