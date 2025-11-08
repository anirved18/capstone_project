#include <iostream>
#include <cstring>
#include <dirent.h>       // For directory listing
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define BUF_SIZE 1024

// Function to list all files in shared_folder
std::string listFiles() {
    DIR* dir;
    struct dirent* entry;
    std::string fileList = "";

    dir = opendir("shared_folder");
    if (!dir) return "Error: Could not open shared_folder.\n";

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) { // Only regular files
            fileList += entry->d_name;
            fileList += "\n";
        }
    }
    closedir(dir);

    if (fileList.empty()) return "No files available.\n";
    return fileList;
}

int main() {
    int server_fd, client_fd;
    sockaddr_in server_addr{}, client_addr{};
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUF_SIZE];

    //  Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Socket creation failed.\n";
        return 1;
    }

    //Set socket options
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Configure address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    //Bind
    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Bind failed.\n";
        close(server_fd);
        return 1;
    }

    // 5️⃣ Listen
    listen(server_fd, 5);
    std::cout << "Server listening on port " << PORT << "...\n";

    // 6️⃣ Accept a client
    client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    std::cout << "Client connected.\n";

    // 7️⃣ Communication loop
    while (true) {
        memset(buffer, 0, BUF_SIZE);
        int bytes = recv(client_fd, buffer, BUF_SIZE - 1, 0);
        if (bytes <= 0) break;

        buffer[bytes] = '\0';
        std::string command(buffer);

        if (command == "LIST") {
            std::string files = listFiles();
            send(client_fd, files.c_str(), files.size(), 0);
        } 
        else if (command.rfind("SELECT ", 0) == 0) {
            std::string filename = command.substr(7);
            std::cout << "Client selected file: " << filename << std::endl;
            std::string msg = "File selected: " + filename + "\n";
            send(client_fd, msg.c_str(), msg.size(), 0);
        } 
        else if (command == "EXIT") {
            std::cout << "Client disconnected.\n";
            break;
        }
    }

    close(client_fd);
    close(server_fd);
    return 0;
}
