#include <radp/radp.h>
#include <string>
#include <sstream>
#include <sys/stat.h>
namespace ghillie575 {

void trimMessage(std::string& message) {
     message.erase(message.find_last_not_of(" \n\r\t") + 1);
}
void sendMessage(int socket, std::string message) {
    send(socket, message.c_str(), message.size(), 0);
}
RADPServer::RADPServer(int port) {
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    this->m_port = port;
    // Create socket
    this->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        logError("Error creating socket");
        return ;
    }

    // Set up the server address structure
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Accept connections from any IP
    serverAddr.sin_port = htons(port); // Convert port number to network byte order

    // Bind the socket to the address
    if (bind(server_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        logError("Error binding socket");
        close(server_socket);
        return;
    }
}
void RADPServer::start() {
    if (listen(server_socket, 5) < 0) {
        logError("Error listening on socket");
        close(server_socket);
        return;
    }
     logInfo("RADP Server listening on port " + std::to_string(m_port));
       while (true) {
        int clientSocket = accept(server_socket, (struct sockaddr*)&clientAddr, &addrLen);
        if (clientSocket < 0) {
            logError("Error accepting client connection");
            continue;
        }

        logClient(clientSocket, "Client connected");

        // Create a new thread to handle the client
        RADPServerClient* client = new RADPServerClient(this->buffer_size);
        clients.push_back(client);
        client->id = clientSocket;
        std::thread clientThread([client, clientSocket]() { client->handle(clientSocket); });
        clientThread.detach(); // Detach the thread to allow it to run independently
    }
}

RADPServer::~RADPServer() {
}
RADPServerClient::RADPServerClient(int buffer_size) {
    this->buffer_size = buffer_size;
}

RADPServerClient::~RADPServerClient() {
}
void RADPServerClient::handle(int socket) {
   char buffer[buffer_size];
    int bytesRead;

    // Communicate with the client
    while ((bytesRead = read(socket, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0'; // Null-terminate the string
        logClient(id, std::string(buffer));
        std::string message(buffer);
        trimMessage(message);
        if (message == "exit") {
            sendMessage(socket, "DISCONNECTED\n");
            break;
        }else if(message == "hello") {
            sendMessage(socket, "HELLO\n");
        }
        else if (message == "getf")
        {
            std::istringstream iss(message);
            std::string command;
            std::string arg;
            if (std::getline(iss, command, ' ') && std::getline(iss, arg, ' ')) {
                if (command == "getf" && !arg.empty()) {
                    std::string fileLocation = "/home/mikhail/RADP/" + arg;
                    
                    struct stat buffer1;
                    if (stat(fileLocation.c_str(), &buffer1) == 0) {
                        std::string extension = arg.substr(arg.find_last_of('.') + 1);
                        std::string fileName = arg.substr(0, arg.find_last_of('.'));
                        sendMessage(socket, "FILE " + fileName + " " + extension + " " + std::to_string(buffer1.st_size) + "\n");
                    }
                    else
                    {
                        sendMessage(socket, "NF\n");
                    }
                }
        }else
        {
            sendMessage(socket, "UKN\n");
        }
        
        
    }
    }

    if (bytesRead < 0) {
        logError("Error reading from client socket");
    }

    close(socket);
    logClient(id, "Client disconnected");
}
}

