#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define BUF_SIZE 1024

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Socket creation failed.\n";
        return 1;
    }

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        std::cerr << "Connection failed.\n";
        return 1;
    }

    std::cout << "Connected to server.\n";

    char buffer[BUF_SIZE];
    std::string command;

    //Request file list
    command = "LIST";
    send(sock, command.c_str(), command.size(), 0);
    memset(buffer, 0, BUF_SIZE);
    recv(sock, buffer, BUF_SIZE - 1, 0);
    std::cout << "\nAvailable files:\n" << buffer << std::endl;

    //  Let user select a file
    std::cout << "Enter filename to select: ";
    std::string filename;
    std::getline(std::cin, filename);
    command = "SELECT " + filename;
    send(sock, command.c_str(), command.size(), 0);

    memset(buffer, 0, BUF_SIZE);
    recv(sock, buffer, BUF_SIZE - 1, 0);
    std::cout << buffer << std::endl;

    //  Exit
    command = "EXIT";
    send(sock, command.c_str(), command.size(), 0);

    close(sock);
    std::cout << "Connection closed.\n";
    return 0;
}
