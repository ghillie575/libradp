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
    enum class RADPCommand
    {
        EXIT,
        DISCONNECTED,
        ERR,
        NF,
        ACSDN,
        DLF,
        DLST,
        DL,
        END,
        UKN
    };
    std::string commandToString(RADPCommand command);
    RADPCommand stringToCommand(const std::string &str);
    void sendMessage(int socket, RADPCommand command);
    class RADPServerClient
    {
    private:
        int m_port;

        int buffer_size = 1024;
        void processCommand(int socket, const std::string &command, const std::vector<std::string> &args);
        void serverDownload(int socket, std::string filename);
        void onDownload();
        void waitForClient(int socket);
    public:
        int id;
        bool connected = false;

        RADPServerClient(int buffer_size);
        ~RADPServerClient();
        void handle(int socket);
        void disconnect(int socket);
    };
    class RADPServer
    {
    private:
        bool running = true;
        int m_port;
        int buffer_size = 1024;
        int server_socket;
        std::function<void()> onDownload;
        std::function<int()> onFileRequested;
        std::vector<RADPServerClient *> clients;
        struct sockaddr_in serverAddr, clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        void clientManaginghread();
        void runServer();
    public:
        RADPServer(int port);
        ~RADPServer();
        RADPServer();
        void start();
        void shutdown();
        void OnDownloadCallback(std::function<void()> callback);
    };
    class RADPClient
    {
    public:
        RADPClient(const std::string &serverAddress, int port);
        ~RADPClient();
        void downloadFile(const std::string &filename);
        void listFiles();
        void getFileInfo(const std::string &filename);
        void getString(const std::string &filename);
        void waitForServer();
        void waitForDownload();
        void disconnect();
        bool connected = false;

    private:
        int socket;
        std::string serverAddress;
        int port;
        bool downloading;
        std::thread receiverThread;
        
        void connectToServer();
        void sendMessage(const std::string &message);
        void sendMessage(RADPCommand command);
        std::vector<std::string> splitMessage(const std::string &message);
        void receiveData();
        long processHeader(const std::string &header, std::ofstream *outputFile);
        void printProgressBar(double percentage, std::string message);
    };
}

#endif