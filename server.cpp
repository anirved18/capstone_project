// server.cpp
// Simple TCP server: accepts one client, receives a greeting, replies back.

#include <arpa/inet.h>   // htons, inet_ntoa, sockaddr_in
#include <netinet/in.h>  // sockaddr_in
#include <sys/socket.h>  // socket, bind, listen, accept, recv, send
#include <sys/types.h>   // basic system data types
#include <unistd.h>      // close
#include <cstring>       // memset, strerror
#include <iostream>      // cout, cerr
#include <csignal>       // signal handling
#include <errno.h>       // errno

int server_fd = -1; // global so signal handler can close it

void handle_sigint(int) {
    if (server_fd != -1) {
        close(server_fd);
        std::cout << "\nServer socket closed. Exiting.\n";
    }
    exit(0);
}

int main() {
    signal(SIGINT, handle_sigint); // ctrl+c -> clean exit

    const int PORT = 8080;
    const int BACKLOG = 5;         // how many pending connections queue will hold
    const int BUF_SIZE = 1024;

    // 1) Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "socket() failed: " << strerror(errno) << "\n";
        return 1;
    }

    // 2) Prepare address structure
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;            // IPv4
    addr.sin_addr.s_addr = INADDR_ANY;    // bind to all interfaces (0.0.0.0)
    addr.sin_port = htons(PORT);          // convert to network byte order

    // (Optional) allow quick reuse of the port after restart
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 3) Bind socket to the address and port
    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) == -1) {
        std::cerr << "bind() failed: " << strerror(errno) << "\n";
        close(server_fd);
        return 1;
    }

    // 4) Start listening
    if (listen(server_fd, BACKLOG) == -1) {
        std::cerr << "listen() failed: " << strerror(errno) << "\n";
        close(server_fd);
        return 1;
    }

    std::cout << "Server listening on port " << PORT << "...\n";

    // 5) Accept a client
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

    // 6) Receive a message from client
    char buffer[BUF_SIZE];
    ssize_t bytes = recv(client_fd, buffer, BUF_SIZE - 1, 0);
    if (bytes == -1) {
        std::cerr << "recv() failed: " << strerror(errno) << "\n";
    } else if (bytes == 0) {
        std::cout << "Client disconnected before sending data.\n";
    } else {
        buffer[bytes] = '\0'; // null-terminate
        std::cout << "Received from client: " << buffer << "\n";

        // 7) Send a reply
        const char *reply = "Hello from server!";
        ssize_t sent = send(client_fd, reply, strlen(reply), 0);
        if (sent == -1) {
            std::cerr << "send() failed: " << strerror(errno) << "\n";
        } else {
            std::cout << "Reply sent (" << sent << " bytes).\n";
        }
    }

    // 8) Clean up
    close(client_fd);
    close(server_fd);
    std::cout << "Server shutdown.\n";
    return 0;
}
