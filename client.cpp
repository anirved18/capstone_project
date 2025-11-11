#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include <errno.h>

#define PORT 8080
#define BUF_SIZE 1024

int main() {
    int sock;
    sockaddr_in serv_addr{};
    char buffer[BUF_SIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Socket creation failed: " << strerror(errno) << "\n";
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        std::cerr << "Connection failed: " << strerror(errno) << "\n";
        return 1;
    }

    std::cout << "Connected to server.\n";

    std::string filename;
    std::cout << "Enter file name to upload: ";
    std::getline(std::cin, filename);

    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Cannot open file '" << filename << "'\n";
        close(sock);
        return 1;
    }

    send(sock, filename.c_str(), filename.size(), 0);
    usleep(100000); // slight delay to avoid mixing filename and data

    while (!file.eof()) {
        file.read(buffer, BUF_SIZE);
        ssize_t bytesRead = file.gcount();
        if (bytesRead > 0)
            send(sock, buffer, bytesRead, 0);
    }

    std::cout << "File '" << filename << "' uploaded successfully.\n";

    file.close();
    close(sock);
    return 0;
}
