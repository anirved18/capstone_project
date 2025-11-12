#include <iostream>
#include <fstream>
#include <cstring>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <openssl/sha.h> 

#define PORT 8080
#define BUF_SIZE 1024

std::string sha256(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)input.c_str(), input.size(), hash);

    std::string hexStr;
    char buf[3];
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(buf, "%02x", hash[i]);
        hexStr += buf;
    }
    return hexStr;
}

bool userExists(const std::string& username, std::string& storedPass) {
    std::ifstream file("users.txt");
    std::string line;
    while (std::getline(file, line)) {
        size_t pos = line.find(':');
        if (pos != std::string::npos) {
            std::string user = line.substr(0, pos);
            std::string pass = line.substr(pos + 1);
            if (user == username) {
                storedPass = pass;
                return true;
            }
        }
    }
    return false;
}

void addUser(const std::string& username, const std::string& password) {
    std::ofstream file("users.txt", std::ios::app);
    file << username << ":" << sha256(password) << "\n";
}

bool handleAuth(int client_fd) {
    char buffer[BUF_SIZE] = {0};

    recv(client_fd, buffer, BUF_SIZE - 1, 0);
    std::string username(buffer);
    std::string storedPass;

    if (userExists(username, storedPass)) {
        std::string msg = "EXIST_USER";
        send(client_fd, msg.c_str(), msg.size(), 0);

        memset(buffer, 0, BUF_SIZE);
        recv(client_fd, buffer, BUF_SIZE - 1, 0);
        std::string password(buffer);

        if (sha256(password) == storedPass) {
            std::string ok = "AUTH_SUCCESS";
            send(client_fd, ok.c_str(), ok.size(), 0);
            return true;
        } else {
            std::string fail = "AUTH_FAIL";
            send(client_fd, fail.c_str(), fail.size(), 0);
            return false;
        }
    } else {
        std::string msg = "NEW_USER";
        send(client_fd, msg.c_str(), msg.size(), 0);

        memset(buffer, 0, BUF_SIZE);
        recv(client_fd, buffer, BUF_SIZE - 1, 0);
        std::string newPass(buffer);
        addUser(username, newPass);

        std::string ok = "NEW_USER_ADDED";
        send(client_fd, ok.c_str(), ok.size(), 0);
        return true;
    }
}

std::string listFiles() {
    DIR* dir = opendir("shared_folder");
    if (!dir) return "Error: Cannot open shared_folder.\n";
    struct dirent* entry;
    std::string files;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG)
            files += entry->d_name + std::string("\n");
    }
    closedir(dir);
    return files.empty() ? "No files available.\n" : files;
}

void sendFile(int client_fd, const std::string& filename) {
    std::ifstream file("shared_folder/" + filename, std::ios::binary);
    if (!file) {
        std::string msg = "ERROR: File not found.\n";
        send(client_fd, msg.c_str(), msg.size(), 0);
        return;
    }

    char buffer[BUF_SIZE];
    while (!file.eof()) {
        file.read(buffer, BUF_SIZE);
        ssize_t bytesRead = file.gcount();
        if (bytesRead > 0)
            send(client_fd, buffer, bytesRead, 0);
    }
    file.close();

    std::string done = "EOF_DONE";
    send(client_fd, done.c_str(), done.size(), 0);
}

void receiveFile(int client_fd, const std::string& filename) {
    std::ofstream out("shared_folder/" + filename, std::ios::binary);
    if (!out) {
        std::cerr << "Cannot create file: " << filename << "\n";
        return;
    }

    char buffer[BUF_SIZE];
    ssize_t bytes;
    while ((bytes = recv(client_fd, buffer, BUF_SIZE, 0)) > 0) {
        if (std::string(buffer, bytes).find("EOF_DONE") != std::string::npos)
            break;
        out.write(buffer, bytes);
        if (bytes < BUF_SIZE) break;
    }
    out.close();
    std::cout << "File uploaded: " << filename << "\n";
}

// ========== MAIN ==========
int main() {
    mkdir("shared_folder", 0777);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Socket creation failed.\n";
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Bind failed.\n";
        close(server_fd);
        return 1;
    }

    listen(server_fd, 5);
    std::cout << "Server listening on port " << PORT << "...\n";

    sockaddr_in client_addr{};
    socklen_t len = sizeof(client_addr);
    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &len);
    if (client_fd == -1) {
        std::cerr << "Accept failed.\n";
        close(server_fd);
        return 1;
    }

    std::cout << "Client connected.\n";

    // AUTH
    char authCmd[BUF_SIZE] = {0};
    recv(client_fd, authCmd, BUF_SIZE - 1, 0);
    if (std::string(authCmd) == "AUTH") {
        bool loggedIn = handleAuth(client_fd);
        if (!loggedIn) {
            std::cout << "Authentication failed. Closing connection.\n";
            close(client_fd);
            close(server_fd);
            return 0;
        }
        std::cout << "User authenticated successfully.\n";
    }

    char buffer[BUF_SIZE];
    while (true) {
        memset(buffer, 0, BUF_SIZE);
        int bytes = recv(client_fd, buffer, BUF_SIZE - 1, 0);
        if (bytes <= 0) break;

        std::string cmd(buffer);

        if (cmd == "LIST") {
            std::string files = listFiles();
            send(client_fd, files.c_str(), files.size(), 0);
        }
        else if (cmd.rfind("DOWNLOAD ", 0) == 0) {
            std::string filename = cmd.substr(9);
            std::cout << "Sending file: " << filename << "\n";
            sendFile(client_fd, filename);
        }
        else if (cmd == "UPLOAD") {
            char namebuf[BUF_SIZE];
            int nameBytes = recv(client_fd, namebuf, BUF_SIZE - 1, 0);
            namebuf[nameBytes] = '\0';
            std::string filename(namebuf);
            std::cout << "Receiving file: " << filename << "\n";
            receiveFile(client_fd, filename);
        }
        else if (cmd == "EXIT") {
            std::cout << "Client disconnected.\n";
            break;
        }
    }

    close(client_fd);
    close(server_fd);
    return 0;
}
