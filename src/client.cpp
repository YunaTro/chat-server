// Клиент подключается к серверу, отправляет и принимает сообщения в отдельных потоках

#include <iostream>
#include <thread>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include "json.hpp"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12345
#define BUFFER_SIZE 1024

using json = nlohmann::json;

void receive_loop(int sockfd) {
    std::string buffer;
    char chunk[BUFFER_SIZE];
    while (true) {
        ssize_t bytes_read = read(sockfd, chunk, BUFFER_SIZE - 1);
        if (bytes_read <= 0) {
            std::cout << "\n[Disconnected from server]\n";
            exit(0);
        }
        chunk[bytes_read] = '\0';
        buffer += chunk;
        
        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos) {
            std::string line = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);
            try {
                auto jmsg = json::parse(line);
                if (jmsg["type"] == "chat") {
                    std::cout << "[" << jmsg["from"].get<std::string>() << "] " << jmsg["message"].get<std::string>() << std::endl;
                } else if (jmsg["type"] == "info") {
                    std::cout << "* " << jmsg["message"].get<std::string>() << std::endl;
                } else if (jmsg["type"] == "private") {
                    std::cout << "[private][" << jmsg["from"].get<std::string>() << "] " << jmsg["message"].get<std::string>() << std::endl;
                }
            } catch (...) {
                std::cout << line << std::endl;
            }
        }
    }
}

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket failed");
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        return 1;
    }

    // чтение приветствия от сервера (ввод имени)
    char init_buffer[BUFFER_SIZE];
    ssize_t n = read(sockfd, init_buffer, BUFFER_SIZE - 1);
    init_buffer[n] = '\0';
    std::cout << init_buffer;

    std::string name;
    std::getline(std::cin, name);
    name += "\n";
    send(sockfd, name.c_str(), name.size(), 0);

    std::thread recv_thread(receive_loop, sockfd);

    std::string msg;
    while (std::getline(std::cin, msg)) {
        send(sockfd, msg.c_str(), msg.size(), 0);
    }

    recv_thread.join();
    close(sockfd);
    return 0;
}
