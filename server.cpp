#include <iostream> 
#include <cstring> 
#include <dirent.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <fstream> 
#include <errno.h> 
 
#define PORT 8080 
#define BUF_SIZE 1024 
 
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
 
    //    Send explicit end marker to stop client loop 
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
 
int main() { 
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