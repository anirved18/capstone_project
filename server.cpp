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
    int server_fd, client_fd;
    sockaddr_in server_addr{}, client_addr{};
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUF_SIZE];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Socket creation failed: " << strerror(errno) << "\n";
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Bind failed: " << strerror(errno) << "\n";
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) == -1) {
        std::cerr << "Listen failed: " << strerror(errno) << "\n";
        close(server_fd);
        return 1;
    }

    std::cout << "Server listening on port " << PORT << "...\n";

    client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd == -1) {
        std::cerr << "Accept failed: " << strerror(errno) << "\n";
        close(server_fd);
        return 1;
    }

    std::cout << "Client connected.\n";

    memset(buffer, 0, BUF_SIZE);
    ssize_t bytes = recv(client_fd, buffer, BUF_SIZE - 1, 0);
    if (bytes <= 0) {
        std::cerr << "Error receiving filename.\n";
        close(client_fd);
        close(server_fd);
        return 1;
    }

    buffer[bytes] = '\0';
    std::string filename(buffer);
    std::string filepath = "shared_folder/" + filename;

    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        std::string errorMsg = "Error: File not found.\n";
        send(client_fd, errorMsg.c_str(), errorMsg.size(), 0);
        std::cerr << "File not found: " << filepath << "\n";
    } else {
        std::cout << "Sending file: " << filename << "\n";
        while (!file.eof()) {
            file.read(buffer, BUF_SIZE);
            ssize_t bytesRead = file.gcount();
            if (bytesRead > 0)
                send(client_fd, buffer, bytesRead, 0);
        }
        std::cout << "File sent successfully.\n";
    }

    file.close();
    close(client_fd);
    close(server_fd);
    std::cout << "Client disconnected.\n";
    return 0;
}
