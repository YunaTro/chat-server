#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <thread>
#include <vector>
#include <mutex>
#include <algorithm>
#include <unordered_map>
#include <fstream>
#include "json.hpp"

#define PORT 12345
#define BUFFER_SIZE 1024
#define LOG_FILE "chat_history.log"

using json = nlohmann::json;

std::mutex cout_mutex;
std::mutex clients_mutex;
std::mutex log_mutex;
std::vector<int> clients;
std::unordered_map<int, std::string> client_names;

void log_message(const json& jmsg) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::ofstream log(LOG_FILE, std::ios::app);
    log << jmsg.dump() << std::endl;
}

void broadcast(const json& jmsg, int sender_fd) {
    std::string msg_str = jmsg.dump() + "\n";
    log_message(jmsg);

    std::lock_guard<std::mutex> lock(clients_mutex);
    for (int client : clients) {
        if (client != sender_fd) {
            send(client, msg_str.c_str(), msg_str.size(), 0);
        }
    } 
}

void send_to_client(int client_fd, const json& jmsg) {
    std::string msg_str = jmsg.dump() + "\n";
    send(client_fd, msg_str.c_str(), msg_str.size(), 0);
}

void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE] = {0};
    ssize_t bytes_read;

    send(client_fd, "Enter your name: ", 18, 0);
    bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);
    if (bytes_read <= 0) {
        close(client_fd);
        return;
    }

    buffer[bytes_read] = '\0';
    std::string name(buffer);
    while (!name.empty() && (name.back() == '\n' || name.back() == '\r')) {
        name.pop_back();
    }

    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        client_names[client_fd] = name;
    }

    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "[Server] " << name << " joined the chat.\n";
    }

    broadcast({{"type", "info"}, {"message", name + " joined the chat"}}, client_fd);

    // Чтение данных клиента
    while ((bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_read] = '\0';
        std::string msg(buffer);
        while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r'))
            msg.pop_back();
        
        if (msg.rfind("/nick ") == 0) {
            std::string new_name = msg.substr(6), old_name;
            {
                std::lock_guard<std::mutex> lock(clients_mutex);
                old_name = client_names[client_fd];
                client_names[client_fd] = new_name;
            }
            broadcast({{"type", "info"}, {"message", old_name + " changed nickname to " + new_name}}, client_fd);
            continue;
        }

        if (msg == "/exit") {
            break;
        }

        if (msg == "/users") {
            json jmsg = {{"type", "info"}, {"message", "Active users:"}};
            send_to_client(client_fd, jmsg);
            {
                std::lock_guard<std::mutex> lock(clients_mutex);
                for (const auto& [fd, uname] : client_names) {
                    json jnames = {{"type", "info"}, {"message", uname}};
                    send_to_client(client_fd, jnames);
                }
            }
            continue;
        }

        if (msg.rfind("/private ") == 0) {
            std::istringstream iss(msg);
            std::string cmd, recipient, private_msg;
            iss >> cmd >> recipient;
            std::getline(iss, private_msg);
            while (!private_msg.empty() && private_msg.front() == ' ') private_msg.erase(0, 1);

            int target_fd = -1;
            {
                std::lock_guard<std::mutex> lock(clients_mutex);
                for (const auto& [fd, uname] : client_names) {
                    if (uname == recipient) {
                        target_fd = fd;
                        break;
                    }
                }
            }
            if (target_fd != -1) {
                json jmsg = {
                    {"type", "private"},
                    {"from", client_names[client_fd]},
                    {"message", private_msg}
                };
                send_to_client(target_fd, jmsg);
            } else {
                send_to_client(client_fd, json{{"type", "info"}, {"message", "User not found"}});
            }
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::lock_guard<std::mutex> lock2(clients_mutex);
            std::cout << "[" << client_names[client_fd] << "] " << msg << std::endl;
        }
        json jmsg;
        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            jmsg = {
                {"type", "chat"},
                {"from", client_names[client_fd]},
                {"message", msg}
            };
        }
        broadcast(jmsg, client_fd);
    }
    
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::lock_guard<std::mutex> lock2(clients_mutex);
        std::cout << "[Server] " << client_names[client_fd] << " disconnected.\n";
    }

    close(client_fd);

    // удаление клиента из списка
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        name = client_names[client_fd];
        clients.erase(std::remove(clients.begin(), clients.end(), client_fd), clients.end());
        client_names.erase(client_fd);
    }
    broadcast({{"type", "info"}, {"message", name + " left the chat"}}, client_fd);
}

int main() {
    int server_fd;
    struct sockaddr_in address;

    // Создание сокета
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        return 1;
    }

    // Привязывание сокета к адресу и порту
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        return 1;
    }

    // Получение входящих подключений
    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        return 1;
    }

    std::cout << "[Server] Listening on port " << PORT << "...\n";

    // Создание потоков для каждого клиента
    std::vector<std::thread> threads;
    while (true) {
        socklen_t addrlen = sizeof(address);
        int client_fd = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "[Server] New client connected.\n";
        }

        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.push_back(client_fd);
        }
        threads.emplace_back(handle_client, client_fd);
    }

    // join все потоки
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    close(server_fd);
    return 0;
}
