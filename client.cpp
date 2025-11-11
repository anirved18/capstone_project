
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
    std::cout << "Enter file name to download: ";
    std::getline(std::cin, filename);

    send(sock, filename.c_str(), filename.size(), 0);

    std::ofstream out("downloaded_" + filename, std::ios::binary);
    if (!out) {
        std::cerr << "Error creating local file.\n";
        return 1;
    }

    ssize_t bytes;
    while ((bytes = recv(sock, buffer, BUF_SIZE, 0)) > 0) {
        out.write(buffer, bytes);
    }

    std::cout << "File downloaded successfully as downloaded_" << filename << "\n";

    out.close();
    close(sock);
    return 0;
}
