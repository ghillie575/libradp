#include <radp/radp.h>
namespace ghillie575 {


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
        if (std::string(buffer) == "exit") {
            break;
        }

        // Echo the message back to the client
        send(socket, buffer, bytesRead, 0);
    }

    if (bytesRead < 0) {
        logError("Error reading from client socket");
    }

    close(socket);
    logClient(id, "Client disconnected");
}
}

