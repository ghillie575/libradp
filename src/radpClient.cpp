#include <radp/radp.h>
namespace ghillie575
{
    RADPClient::RADPClient(const std::string &serverAddress, int port)
        : serverAddress(serverAddress), port(port), downloading(false)
    {
        connectToServer();
        receiverThread = std::thread(&RADPClient::receiveData, this);
    }
    RADPClient::~RADPClient()
    {
        downloading = false;
        if (receiverThread.joinable())
        {
            receiverThread.join();
        }
        close(socket);
    }

    void RADPClient::connectToServer()
    {
        socket = ::socket(AF_INET, SOCK_STREAM, 0);
        if (socket < 0)
        {
            throw std::runtime_error("Error creating socket");
        }

        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        if (inet_pton(AF_INET, serverAddress.c_str(), &serverAddr.sin_addr) <= 0)
        {
            throw std::runtime_error("Invalid address/ Address not supported");
        }

        if (connect(socket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        {
            throw std::runtime_error("Connection failed");
        }
        connected = true;
    }

    void RADPClient::sendMessage(const std::string &message)
    {
        send(socket, message.c_str(), message.size(), 0);
    }

    std::vector<std::string> RADPClient::splitMessage(const std::string &message)
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
    void RADPClient::receiveData()
    {
        char buffer[2048];
        std::ofstream outputFile;
        while (true)
        {
            ssize_t bytesRead = recv(socket, buffer, sizeof(buffer), 0);
            if (bytesRead <= 0)
            {
                break;
            }

            std::string data(buffer, bytesRead);
            if (data == "DLF")
            {
                downloading = false;
                std::cout << "Download complete\n";
                if (outputFile.is_open())
                {
                    outputFile.close();
                }
            }
            if (downloading)
            {

                size_t headerEnd = data.find("#radpdl");
                if (headerEnd != std::string::npos)
                {
                    std::string header = data.substr(0, headerEnd + 2);
                    processHeader(header, &outputFile);
                    data = data.substr(headerEnd + 2);
                }
                else
                {
                    if (outputFile.is_open())
                    {
                        outputFile.write(data.c_str(), data.size());
                        if (!outputFile)
                        {
                            std::cerr << "Error writing to file" << std::endl;
                        }
                        else
                        {
                            std::cout << "Data written to file\n";
                            sendMessage("OK\n");
                        }
                    }
                    else
                    {
                        std::cerr << "File not open\n";
                        sendMessage("ERR\n");
                    }
                }
            }
            else
            {
                trimMessage(data);
                if (data == "DISCONNECTED")
                {
                    break;
                }
                else if (data == "ACSDN")
                {
                    std::cout << "Access Denied\n";
                }
                else if (data == "UKN")
                {
                    std::cout << "Unknown command\n";
                }
                else if (data == "NF")
                {
                    std::cout << "File not found\n";
                }
                else if (data == "DLST")
                {
                    downloading = true;
                }
                else
                {
                    std::cout << data << "\n";
                }
            }
        }

        if (outputFile.is_open())
        {
            outputFile.close();
        }
        connected = false;
    }

    void RADPClient::processHeader(const std::string &header, std::ofstream *outputFile)
    {
        std::vector<std::string> tokens = splitMessage(header);
        if (tokens.size() < 6 || tokens[0] != "radpdl#")
        {
            throw std::runtime_error("Invalid header format");
        }

        // Debugging output
        std::cout << "Header tokens: ";
        for (const auto &token : tokens)
        {
            std::cout << token << " ";
        }
        std::cout << std::endl;

        long totalBytesSent = std::stol(tokens[1]);
        long fileSize = std::stol(tokens[2]);
        size_t chunkSize = std::stoul(tokens[3]);
        std::string filename = tokens[4];

        std::cout << "Downloading: " << totalBytesSent << "/" << fileSize << " bytes\n";

        if (totalBytesSent == 0)
        {
            filename.append(".radpd");
            if (!outputFile->is_open())
            {
                outputFile->open(filename, std::ios::binary);
                if (!outputFile->is_open())
                {
                    throw std::runtime_error("Failed to open destination file");
                }
            }
        }
    }

    void RADPClient::downloadFile(const std::string &filename)
    {
        sendMessage("DL " + filename + "\n");
    }

    void RADPClient::listFiles()
    {
        sendMessage("LS");
    }

    void RADPClient::getFileInfo(const std::string &filename)
    {
        sendMessage("GETF " + filename + "\n");
    }
}