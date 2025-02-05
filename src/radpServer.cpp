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
#include <sys/socket.h>
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
    void RADPServerClient::serverDownload(int socket, std::string filename)
    {
        std::string path = radpServerDir + "/" + filename;

        // Check if the file path attempts to access parent directory
        if (path.find("..") != std::string::npos)
        {
            sendMessage(socket, "ACSDN\n");
            return;
        }
        std::cout << "Serving file: " << path << std::endl;
        std::ifstream fileStream(path, std::ios::binary);
        if (!fileStream)
        {
            sendMessage(socket, "NF\n");
        }
        else
        {
            sendMessage(socket, "DLST\n");
            sleep(1); // Wait for client to prepare for download
            fileStream.seekg(0, std::ios::end);
            long fileSize = fileStream.tellg();
            fileStream.seekg(0, std::ios::beg);

            const size_t chunkSize = 1024; // Define the chunk size
            std::vector<char> buffer(chunkSize);
            long totalBytesSent = 0;

            while (fileStream.read(buffer.data(), chunkSize) || fileStream.gcount() > 0)
            {
                size_t bytesRead = fileStream.gcount();
                // Create header with progress information
                std::string header = "radpdl# " + std::to_string(totalBytesSent) + " " + std::to_string(fileSize) + " " + std::to_string(chunkSize) + " " + filename + " #radpdl";
                logClient(socket, "Chunk send: " + header + "\n");
                send(socket, header.c_str(), header.size(), 0);

                send(socket, buffer.data(), bytesRead, 0);
                totalBytesSent += bytesRead;
                // Wait for client to send "OK"
                waitForClient(socket);
            }

            logClient(socket, "File send finished");
            sendMessage(socket, "DLF");
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
    void RADPServerClient::serverListFiles(int socket)
    {

        std::string result = "";
        struct stat fileStat;
        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir(radpServerDir.c_str())) != NULL)
        {
            while ((ent = readdir(dir)) != NULL)
            {
                if (stat(ent->d_name, &fileStat) == -1)
                {
                    continue;
                }
                result += ent->d_name;
                result += "\n";
            }
            closedir(dir);
        }
        sendMessage(socket, result);
    }
    void RADPServerClient::serverGetFileInfo(int socket, std::string filename)
    {
        std::string fullPath = radpServerDir + "/" + filename;
        struct stat fileStat;
        if (stat(fullPath.c_str(), &fileStat) == 0)
        {
            if (S_ISREG(fileStat.st_mode))
            {
                sendMessage(socket, "FILE " + std::to_string(fileStat.st_size) + "\n");
            }
            else
            {
                sendMessage(socket, "NF\n");
            }
        }
        else
        {
            sendMessage(socket, "NF\n");
        }
    }
    void RADPServerClient::disconnect(int socket)
    {
        connected = false;
        sendMessage(socket, "DISCONNECTED\n");
    }
    void RADPServerClient::serverReadString(int socket, std::string filename)
    {
        std::string path = radpServerDir + filename;
        if (path.find("..") != std::string::npos)
        {
            sendMessage(socket, "ACSDN\n");
            return;
        }
        FILE *file = fopen(path.c_str(), "r");
        if (file == NULL)
        {
            sendMessage(socket, "NF\n");
        }
        else
        {
            fseek(file, 0, SEEK_END);
            long fileSize = ftell(file);
            rewind(file);
            char *buffer = (char *)malloc(fileSize);
            fread(buffer, 1, fileSize, file);
            sendMessage(socket, std::string(buffer));
            fclose(file);
        }
        
    }
    void RADPServerClient::processCommand(int socket, const std::string &command, const std::vector<std::string> &args)
    {
        if (command == "EXIT")
        {
            disconnect(socket);
        }
        else if (command == "LS")
        {
            serverListFiles(socket);
        }
        else if (command == "GETS")
        {
            if (args.size() == 1)
            {
                std::string filename = args[0];
                serverReadString(socket, filename);
            }

            else
            {
                sendMessage(socket, "ERR\n");
            }
        }
        else if (command == "DL")
        {
            if (args.size() == 1)
            {
                std::string filename = args[0];
                onDownload();
                serverDownload(socket, filename);
                disconnect(socket);
            }
        }
        else if (command == "GETF")
        {
            if (args.empty())
            {
                sendMessage(socket, "ERR\n");
                
                return;
            }
            std::string filePath = args[0];

            // Check if the file path attempts to access parent directory
            if (filePath.find("..") != std::string::npos)
            {
                sendMessage(socket, "ACSDN\n");
                return;
            }
            serverGetFileInfo(socket, filePath);
        }
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
    void RADPServer::start()
    {
        if (listen(server_socket, 5) < 0)
        {
            logError("Error listening on socket");
            close(server_socket);
            return;
        }
        logInfo("RADP Server listening on port " + std::to_string(m_port));
        while (true)
        {
            int clientSocket = accept(server_socket, (struct sockaddr *)&clientAddr, &addrLen);
            if (clientSocket < 0)
            {
                logError("Error accepting client connection");
                continue;
            }

            logClient(clientSocket, "Client connected");

            // Create a new thread to handle the client
            RADPServerClient *client = new RADPServerClient(this->buffer_size);
            clients.push_back(client);
            client->id = clientSocket;
            std::thread clientThread([client, clientSocket]()
                                     { client->handle(clientSocket); });
            clientThread.detach(); // Detach the thread to allow it to run independently
        }
    }

    RADPServer::~RADPServer()
    {
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
                if (command == "EXIT")
                {
                    break;
                }
            }
        }

        if (bytesRead < 0)
        {
            logError("Error reading from client socket");
        }

        close(socket);
        logClient(id, "Client disconnected");
    }
}
