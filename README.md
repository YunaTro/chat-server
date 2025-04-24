# Chat Server on C++

A multithreaded chat server and CLI client written in C++. Supports nicknames, public and private messages, user list command, and logs all messages to a file in JSON format.

---

## 🔧 Features

- TCP-based server and client
- Command support: `/nick`, `/exit`, `/users`, `/private <user> <message>`
- Broadcast and private messaging
- Chat history saved to `chat_history.log`
- JSON-based message protocol using `nlohmann/json`
- CLI client with real-time message receiving

---

## 📁 Project structure

```
chat-server/
├── include/
│   └── json.hpp
├── src/
│   ├── server.cpp
│   ├── client.cpp
│   └── test_parser.cpp       # unit tests
├── chat_history.log          # created at runtime
├── CMakeLists.txt
├── build/                    # created during compilation
└── README.md
```

---

## ⚙️ Build instructions

### ✅ Prerequisites

- C++17 compatible compiler (e.g., `g++`, `clang++`)
- CMake >= 3.10

### 🔨 Build

```bash
cd chat-server
mkdir -p build
cd build
cmake ..
make
```

This creates two executables:

- `server`
- `client`

---

## 🚀 Run

### Server

```bash
./server
```

### Client

```bash
./client
```

You can launch multiple client instances in separate terminals.

💡 Log-файл chat_history.log сохраняется в ту директорию, откуда вы запускаете сервер. Если хотите видеть его в корне проекта, запускайте ./build/server из chat-server/.

---

## ✅ Commands

```
/nick <name>        Change your nickname
/exit               Disconnect from the chat
/users              List all active users
/private <name> <msg>  Send a private message
```

---

## 🧪 Unit Tests (parser example)

Simple test file: `src/test_parser.cpp`

Compile with:

```bash
g++ -std=c++17 -Iinclude src/test_parser.cpp -o test_parser
./test_parser
```

