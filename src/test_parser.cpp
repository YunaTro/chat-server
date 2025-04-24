#include <iostream>
#include <sstream>
#include <cassert>
#include <string>

// Простая функция парсинга команды /private <user> <msg>
void parse_private_command(const std::string& input, std::string& recipient, std::string& message) {
    std::istringstream iss(input);
    std::string cmd;
    iss >> cmd >> recipient;
    std::getline(iss, message);
    while (!message.empty() && message.front() == ' ') message.erase(0, 1);
}

// Тест функции parse_private_command
void test_parse_private_command() {
    std::string recipient, message;
    parse_private_command("/private Alice Hello there!", recipient, message);
    assert(recipient == "Alice");
    assert(message == "Hello there!");

    parse_private_command("/private Bob    How are you?", recipient, message);
    assert(recipient == "Bob");
    assert(message == "How are you?");

    parse_private_command("/private Charlie     Hey", recipient, message);
    assert(recipient == "Charlie");
    assert(message == "Hey");

    std::cout << "All /private command tests passed.\n";
}

void test_nick_command() {
    std::string cmd = "/nick NewName";
    assert(cmd.rfind("/nick ", 0) == 0);
    std::string new_name = cmd.substr(6);
    assert(new_name == "NewName");
    std::cout << "Nick command test passed.\n";
}

int main() {
    test_parse_private_command();
    test_nick_command();
    std::cout << "All tests passed successfully.\n";
    return 0;
}
