

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <errno.h>

int main() {
    const char *SERVER_IP = "127.0.0.1";
    const int PORT = 8080;
    const int BUF_SIZE = 1024;

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == -1) {
        std::cerr << "socket() failed: " << strerror(errno) << "\n";
        return 1;
    }

    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        std::cerr << "inet_pton() failed for " << SERVER_IP << "\n";
        close(sock);
        return 1;
    }

    if (connect(sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)

    {
        std::cerr << "connect() failed: " << strerror(errno) << "\n";
        close(sock);
        return 1;
    }
    std::cout << "Connected to server " << SERVER_IP << ":" << PORT << "\n";

    const char *msg = "Hello from client!";
    if (send(sock, msg, strlen(msg), 0) == -1) {
        std::cerr << "send() failed: " << strerror(errno) << "\n";
        close(sock);
        return 1;
    }
    std::cout << "Message sent to server.\n";

    char buffer[BUF_SIZE];
    ssize_t bytes = recv(sock, buffer, BUF_SIZE - 1, 0);
    if (bytes == -1) {
        std::cerr << "recv() failed: " << strerror(errno) << "\n";
    } else if (bytes == 0) {
        std::cout << "Server closed connection.\n";
    } else {
        buffer[bytes] = '\0';
        std::cout << "Received from server: " << buffer << "\n";
    }

    close(sock);
    return 0;
}