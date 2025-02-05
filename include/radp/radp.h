#include <functional>
#include <iostream>
#include <cstring>
#include <thread>
#include <vector>
#include <sstream>
#include <fstream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include <io.h>
#define close _close
typedef int ssize_t;
#include <windows.h>
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif
#ifndef RADP_H
#define RADP_H

namespace ghillie575
{
    extern std::string radpServerDir;
    void logClient(int id, std::string message);
    void logInfo(std::string message);
    void logError(std::string message);
    void setRadpServerDir(std::string dir);
    void trimMessage(std::string &message);
    void sendMessage(int socket, std::string message);
    class RADPServerClient
    {
    private:
        int m_port;

        int buffer_size = 1024;
        bool connected = false;
        void processCommand(int socket, const std::string &command, const std::vector<std::string> &args);
        void serverDownload(int socket, std::string filename);
        void onDownload();
        void waitForClient(int socket);
        void serverListFiles(int socket);
        void serverGetFileInfo(int socket, std::string filename);
        void disconnect(int socket);
        void serverReadString(int socket, std::string filename);

    public:
        int id;
        RADPServerClient(int buffer_size);
        ~RADPServerClient();
        void handle(int socket);
        };
    class RADPServer
    {
    private:
        int m_port;
        int buffer_size = 1024;
        int server_socket;
        std::function<void()> onDownload;
        std::function<int()> onFileRequested;
        std::vector<RADPServerClient *> clients;
        struct sockaddr_in serverAddr, clientAddr;
        socklen_t addrLen = sizeof(clientAddr);

    public:
        RADPServer(int port);
        ~RADPServer();
        void start();
        void OnDownloadCallback(std::function<void()> callback);
        void FileRequestedCallback(std::function<int()> callback);
    };
    class RADPClient
    {
    public:
        RADPClient(const std::string &serverAddress, int port);
        ~RADPClient();
        void downloadFile(const std::string &filename);
        void listFiles();
        void getFileInfo(const std::string &filename);
        
        bool connected = false;

    private:
        int socket;
        std::string serverAddress;
        int port;
        bool downloading;
        std::thread receiverThread;
        void waitForServer();
        void connectToServer();
        void sendMessage(const std::string &message);
        std::vector<std::string> splitMessage(const std::string &message);
        void receiveData();
        long processHeader(const std::string &header, std::ofstream *outputFile);
        void printProgressBar(double percentage,std::string message);
    };
}

#endif