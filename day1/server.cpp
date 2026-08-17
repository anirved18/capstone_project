

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <csignal>
#include <errno.h>

int server_fd = -1;

void handle_sigint(int) {
    if (server_fd != -1) {
        close(server_fd);
        std::cout << "\nServer socket closed. Exiting.\n";
    }
    exit(0);
}

int main() {
    signal(SIGINT, handle_sigint);

    const int PORT = 8080;
    const int BACKLOG = 5;
    const int BUF_SIZE = 1024;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1) {
        std::cerr << "socket() failed: " << strerror(errno) << "\n";
        return 1;

    }

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;

    addr.sin_addr.s_addr = INADDR_ANY;

    addr.sin_port = htons(PORT);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) == -1)

    {
        std::cerr << "bind() failed: " << strerror(errno) << "\n";
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, BACKLOG) == -1) {
        std::cerr << "listen() failed: " << strerror(errno) << "\n";
        close(server_fd);
        return 1;
    }

    std::cout << "Server listening on port " << PORT << "...\n";

    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);

    if (client_fd == -1) {
        std::cerr << "accept() failed: " << strerror(errno) << "\n";
        close(server_fd);
        return 1;
    }

    char client_ip[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));

    std::cout << "Client connected: " << client_ip << ":" << ntohs(client_addr.sin_port) << "\n";

    char buffer[BUF_SIZE];
    ssize_t bytes = recv(client_fd, buffer, BUF_SIZE - 1, 0);

    if (bytes == -1) {
        std::cerr << "recv() failed: " << strerror(errno) << "\n";
    } else if (bytes == 0) {
        std::cout << "Client disconnected before sending data.\n";
    } else {
        buffer[bytes] = '\0';
        std::cout << "Received from client: " << buffer << "\n";

        const char *reply = "Hello from server!";
        ssize_t sent = send(client_fd, reply, strlen(reply), 0);
        if (sent == -1) {
            std::cerr << "send() failed: " << strerror(errno) << "\n";
        } else {
            std::cout << "Reply sent (" << sent << " bytes).\n";
        }
    }

    close(client_fd);
    close(server_fd);
    std::cout << "Server shutdown.\n";
    return 0;
}

