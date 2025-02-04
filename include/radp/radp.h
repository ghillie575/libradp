#include <functional>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <vector>
#ifndef RADP_H
#define RADP_H
namespace ghillie575 {
    void logClient(int id, std::string message) {
    std::cout << "[CLIENT " << id << "] " << message << std::endl;
}
void logInfo(std::string message) {
    std::cout << "[INFO] " << message << std::endl;
}
void logError(std::string message) {
    std::cerr << "[ERROR] " << message << std::endl;
}
void trimMessage(std::string& message) ;
void sendMessage(int socket, std::string message);
    class RADPServerClient {
        private:
            int m_port;
            
            int buffer_size = 1024;
        public:
            int id;
            RADPServerClient(int buffer_size);
            ~RADPServerClient();
            void handle(int socket);

    };
    class RADPServer {
        private:
            int m_port;
            int buffer_size = 1024;
            int server_socket;
            std::function<void()> onDownload;
            std::function<int()> onFileRequested;
            std::vector <RADPServerClient*> clients;
            struct sockaddr_in serverAddr, clientAddr;
            socklen_t addrLen = sizeof(clientAddr);
        public:
            RADPServer(int port);
            ~RADPServer();
            void start();
            void OnDownloadCallback(std::function<void()> callback);
            void FileRequestedCallback(std::function<int()> callback);

    };
    
}

#endif