#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <termios.h>  // ✅ For hiding password input

#define PORT 8080
#define BUF_SIZE 1024

// ========== Function to Hide Password ==========
std::string getHiddenPassword() {
    std::string password;
    struct termios oldt, newt;

    // Turn off terminal echo
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    std::getline(std::cin, password);

    // Restore terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    std::cout << std::endl;
    return password;
}

// ========== File Transfer Functions ==========
void sendFile(int sock, const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Cannot open file '" << filename << "'\n";
        return;
    }
    send(sock, filename.c_str(), filename.size(), 0);
    usleep(100000);
    char buffer[BUF_SIZE];
    while (!file.eof()) {
        file.read(buffer, BUF_SIZE);
        ssize_t bytesRead = file.gcount();
        if (bytesRead > 0)
            send(sock, buffer, bytesRead, 0);
    }
    file.close();
    std::string done = "EOF_DONE";
    send(sock, done.c_str(), done.size(), 0);
    std::cout << "File '" << filename << "' uploaded successfully.\n";
}

void receiveFile(int sock, const std::string& filename) {
    std::ofstream out("downloaded_" + filename, std::ios::binary);
    if (!out) {
        std::cerr << "Error creating local file.\n";
        return;
    }

    char buffer[BUF_SIZE];
    ssize_t bytes;
    while ((bytes = recv(sock, buffer, BUF_SIZE, 0)) > 0) {
        if (std::string(buffer, bytes).find("EOF_DONE") != std::string::npos)
            break;
        out.write(buffer, bytes);
        if (bytes < BUF_SIZE) break;
    }

    out.close();
    std::cout << "File downloaded successfully as downloaded_" << filename << "\n";
}

// ========== Authentication Function ==========
bool authenticate(int sock) {
    std::string cmd = "AUTH";
    send(sock, cmd.c_str(), cmd.size(), 0);

    std::string username;
    std::cout << "Enter username: ";
    std::getline(std::cin, username);
    send(sock, username.c_str(), username.size(), 0);

    char buffer[BUF_SIZE] = {0};
    recv(sock, buffer, BUF_SIZE - 1, 0);
    std::string response(buffer);

    if (response == "NEW_USER") {
        std::cout << "New user! Set a password: ";
        std::string password = getHiddenPassword();
        send(sock, password.c_str(), password.size(), 0);
        recv(sock, buffer, BUF_SIZE - 1, 0);
        std::cout << "Account created successfully!\n";
        return true;
    } 
    else if (response == "EXIST_USER") {
        std::cout << "Enter password: ";
        std::string password = getHiddenPassword();
        send(sock, password.c_str(), password.size(), 0);
        recv(sock, buffer, BUF_SIZE - 1, 0);
        std::string result(buffer);
        if (result == "AUTH_SUCCESS") {
            std::cout << "Login successful!\n";
            return true;
        } else {
            std::cout << "Invalid password.\n";
            return false;
        }
    }
    return false;
}

// ========== MAIN ==========
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

    if (!authenticate(sock)) {
        std::cout << "Authentication failed. Exiting.\n";
        close(sock);
        return 0;
    }

    while (true) {
        std::cout << "\n1. List Files\n2. Download File\n3. Upload File\n4. Exit\nEnter choice: ";
        int choice;
        std::cin >> choice;
        std::cin.ignore();

        if (choice == 1) {
            std::string cmd = "LIST";
            send(sock, cmd.c_str(), cmd.size(), 0);
            char buffer[BUF_SIZE] = {0};
            recv(sock, buffer, BUF_SIZE - 1, 0);
            std::cout << "\nAvailable files:\n" << buffer << std::endl;
        } 
        else if (choice == 2) {
            std::string filename;
            std::cout << "Enter filename to download: ";
            std::getline(std::cin, filename);
            std::string cmd = "DOWNLOAD " + filename;
            send(sock, cmd.c_str(), cmd.size(), 0);
            receiveFile(sock, filename);
        } 
        else if (choice == 3) {
            std::string filename;
            std::cout << "Enter filename to upload: ";
            std::getline(std::cin, filename);
            std::string cmd = "UPLOAD";
            send(sock, cmd.c_str(), cmd.size(), 0);
            usleep(100000);
            sendFile(sock, filename);
        } 
        else if (choice == 4) {
            std::string cmd = "EXIT";
            send(sock, cmd.c_str(), cmd.size(), 0);
            break;
        } 
        else {
            std::cout << "Invalid choice.\n";
        }
    }

    close(sock);
    std::cout << "Disconnected from server.\n";
    return 0;
}
