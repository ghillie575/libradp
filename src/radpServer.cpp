#include <radp/radp.h>
#include <string>
#include <sstream>
#include <sys/stat.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <io.h>
#define sleep(x) Sleep(1000 * (x))
#define read _read
#define close closesocket
#else
#include <dirent.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <chrono>
#include <fcntl.h>
#endif
namespace ghillie575
{
    std::string radpServerDir = ".";
    void logClient(int id, std::string message)
    {
        std::cout << "[CLIENT " << id << "] " << message << std::endl;
    }
    void logInfo(std::string message)
    {
        std::cout << "[INFO] " << message << std::endl;
    }
    void logError(std::string message)
    {
        std::cerr << "[ERROR] " << message << std::endl;
    }
    void setRadpServerDir(std::string dir)
    {
        radpServerDir = dir;
    }
    void trimMessage(std::string &message)
    {
        message.erase(message.find_last_not_of("\n\r\t") + 1);
    }
    void sendMessage(int socket, std::string message)
    {
        send(socket, message.c_str(), message.size(), 0);
    }
    std::vector<std::string> splitMessage(const std::string &message)
    {
        std::istringstream iss(message);
        std::vector<std::string> tokens;
        std::string token;
        while (iss >> token)
        {
            tokens.push_back(token);
        }
        return tokens;
    }
    std::string commandToString(RADPCommand command)
    {
        switch (command)
        {
        case RADPCommand::EXIT:
            return "EXIT";
        case RADPCommand::DISCONNECTED:
            return "DISCONNECTED";
        case RADPCommand::ERR:
            return "ERR";
        case RADPCommand::NF:
            return "NF";
        case RADPCommand::ACSDN:
            return "ACSDN";
        case RADPCommand::DLF:
            return "DLF";
        case RADPCommand::DLST:
            return "DLST";
        case RADPCommand::DL:
            return "DL";
        case RADPCommand::END:
            return "EOF";
        default:
            return "UKN";
        }
    }
    RADPCommand stringToCommand(const std::string &commandStr)
    {
        if (commandStr == "EXIT")
            return RADPCommand::EXIT;
        else if (commandStr == "DISCONNECTED")
            return RADPCommand::DISCONNECTED;
        else if (commandStr == "ERR")
            return RADPCommand::ERR;
        else if (commandStr == "NF")
            return RADPCommand::NF;
        else if (commandStr == "ACSDN")
            return RADPCommand::ACSDN;
        else if (commandStr == "DLF")
            return RADPCommand::DLF;
        else if (commandStr == "DLST")
            return RADPCommand::DLST;
        else if (commandStr == "DL")
            return RADPCommand::DL;
        else if (commandStr == "EOF")
            return RADPCommand::END;
        else
            return RADPCommand::UKN;
    }
    void sendMessage(int socket, RADPCommand command)
    {
        sendMessage(socket, commandToString(command) + "\n");
    }

    void RADPServerClient::serverDownload(int socket, std::string filename)
    {
        std::string path = radpServerDir + "/" + filename;

        // Check if the file path attempts to access parent directory
        if (path.find("..") != std::string::npos)
        {
            sendMessage(socket, RADPCommand::ACSDN);
            return;
        }
        std::cout << "Serving file: " << path << std::endl;
        std::ifstream fileStream(path, std::ios::binary);
        if (!fileStream)
        {
            sendMessage(socket, RADPCommand::NF);
        }
        else
        {
            sendMessage(socket, RADPCommand::DLST);
            sleep(1); // Wait for client to prepare for download
            fileStream.seekg(0, std::ios::end);
            long fileSize = fileStream.tellg();
            fileStream.seekg(0, std::ios::beg);

            const size_t chunkSize = (1024 * 1024); // Define the chunk size
            std::vector<char> buffer(chunkSize);
            long totalBytesSent = 0;
            std::string header = "##dl## " + std::to_string(fileSize) + " " + filename + " ##dl##";
            logClient(socket, "Header sent: " + header + "\n");
            send(socket, header.c_str(), header.size(), 0);
            sleep(1); // Wait for client to prepare for download
            while (fileStream.read(buffer.data(), chunkSize) || fileStream.gcount() > 0)
            {
                size_t bytesRead = fileStream.gcount();
                send(socket, buffer.data(), bytesRead, 0);
                totalBytesSent += bytesRead;
            }
            logClient(socket, "File send finished");
            sleep(1); // Wait for client to finish downloading
            sendMessage(socket, RADPCommand::DLF);
            std::cout << "File sent: " << totalBytesSent << " bytes\n";
            fileStream.close();
        }
    }
    void RADPServerClient::onDownload()
    {
    }
    void RADPServerClient::waitForClient(int socket)
    {
        char buffer[1024];
        size_t bytesRead;
        while ((bytesRead = read(socket, buffer, sizeof(buffer) - 1)) > 0)
        {
            buffer[bytesRead] = '\0'; // Null-terminate the string
            std::string message(buffer);
            trimMessage(message);
            if (message == "OK")
            {
                break;
            }
        }
    }
    void RADPServerClient::disconnect(int socket)
    {
        connected = false;
        sendMessage(socket, RADPCommand::DISCONNECTED);
    }
    void RADPServerClient::processCommand(int socket, const std::string &command, const std::vector<std::string> &args)
    {
        RADPCommand cmd = stringToCommand(command);
        if (cmd == RADPCommand::EXIT)
        {
            disconnect(socket);
        }
        else if (cmd == RADPCommand::DL)
        {
            if (args.size() == 1)
            {
                std::string filename = args[0];
                onDownload();
                serverDownload(socket, filename);
            }
        }
        else
        {
            sendMessage(socket, RADPCommand::ERR);
        }
    }
    RADPServer::RADPServer()
    {
    }
    void RADPServer::shutdown()
    {
        for (auto client : clients)
        {
            client->disconnect(client->id);
            std::cout << "Disconnecting client " << client->id << std::endl;
        }
        std::cout << "Shutting down server\n";
        close(server_socket);
        running = false;
    }
    RADPServer::RADPServer(int port)
    {
        struct sockaddr_in serverAddr, clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        this->m_port = port; 
        // Create socket
        this->server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0)
        {
            logError("Error creating socket");
            return;
        }

        // Set up the server address structure
        int flag = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(int)); // Allow socket reuse
        setsockopt(server_socket, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)); // Set TCP_NODELAY
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY; // Accept connections from any IP
        serverAddr.sin_port = htons(port);       // Convert port number to network byte order

        // Bind the socket to the address
        if (bind(server_socket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        {
            logError("Error binding socket");
            close(server_socket);
            return;
        }
    }
    void RADPServer::clientManaginghread()
    {
        while (running)
        {
            for (auto client : clients)
            {
                if (!client->connected)
                {
                    std::cout << "Removing unused resources for client: " << client->id << "\n";
                    clients.erase(std::remove(clients.begin(), clients.end(), client), clients.end());
                    delete client;
                }
            }
            sleep(1);
        }
    }
    void RADPServer::runServer()
    {
        // Set the server socket to non-blocking mode
        int flags = fcntl(server_socket, F_GETFL, 0);
        fcntl(server_socket, F_SETFL, flags | O_NONBLOCK);

        while (running)
        {
            int clientSocket = accept(server_socket, (struct sockaddr *)&clientAddr, &addrLen);
            if (clientSocket < 0)
            {
                if (errno == EWOULDBLOCK)
                {
                    // No incoming connections, continue
                    usleep(100000); // Sleep for 100ms to avoid busy-waiting
                    continue;
                }
                if (running)
                {
                    logError("Error accepting client connection");
                }
                break;
            }
            int flag = 1;
            setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

            logClient(clientSocket, "Client connected");

            // Create a new thread to handle the client
            RADPServerClient *client = new RADPServerClient(this->buffer_size);
            clients.push_back(client);
            client->id = clientSocket;
            std::thread clientThread([client, clientSocket]()
                                     { client->handle(clientSocket); });
            clientThread.detach(); // Detach the thread to allow it to run independently
        }
        close(server_socket);
    }
    void RADPServer::start()
    {

        if (listen(server_socket, 5) < 0)
        {
            logError("Error listening on socket");
            close(server_socket);
            return;
        }
        logInfo("RADP Server listening on port " + std::to_string(m_port));
        std::thread clientManagerThread([this]()
                                        { clientManaginghread(); });
        runServer();
        clientManagerThread.join();
        return;
    }
    RADPServer::~RADPServer()
    {
        shutdown();
    }
    RADPServerClient::RADPServerClient(int buffer_size)
    {
        this->buffer_size = buffer_size;
    }

    RADPServerClient::~RADPServerClient()
    {
    }

    void RADPServerClient::handle(int socket)
    {
        char buffer[buffer_size];
        int bytesRead;

        // Communicate with the client
        connected = true;
        while (((bytesRead = read(socket, buffer, sizeof(buffer) - 1)) > 0) && connected)
        {
            buffer[bytesRead] = '\0'; // Null-terminate the string
            logClient(id, std::string(buffer));
            std::string message(buffer);
            trimMessage(message);
            std::vector<std::string> tokens = splitMessage(message);
            if (!tokens.empty())
            {
                std::string command = tokens[0];
                std::vector<std::string> args(tokens.begin() + 1, tokens.end());
                processCommand(socket, command, args);
                if (connected == false)
                {
                    close(socket);
                }
            }
        }

        if (connected && bytesRead < 0)
        {
            logError("Error reading from client socket");
        }

        close(socket);
        logClient(id, "Client disconnected");
        connected = false;
    }
}
