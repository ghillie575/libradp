#include <radp/radp.h>
#include <iomanip>
#include <netinet/tcp.h> // Include this header for TCP_NODELAY
namespace ghillie575
{

    void RADPClient::printProgressBar(double percentage, std::string message)
    {
        int barWidth = 50;                    // Width of the progress bar
        std::cout << "\r" << message << " ["; // Carriage return to overwrite the line

        int pos = static_cast<int>(barWidth * (percentage / 100.0));
        for (int i = 0; i < barWidth; ++i)
        {
            if (i < pos)
                std::cout << "=";
            else if (i == pos)
                std::cout << ">";
            else
                std::cout << " ";
        }

        std::cout << "] " << std::fixed << std::setprecision(2) << percentage << "% " << std::flush;
    }
    void RADPClient::waitForDownload()
    {
        while (downloading)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
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
    void RADPClient::waitForServer()
    {
        char buffer[1024];
        ssize_t bytesRead;
        while ((bytesRead = read(socket, buffer, sizeof(buffer) - 1)) > 0)
        {
            buffer[bytesRead] = '\0'; // Null-terminate the string
            std::string message(buffer);
            trimMessage(message);
            if (message == "EOF")
            {
                break;
            }
        }
    }
    void RADPClient::connectToServer()
    {
        socket = ::socket(AF_INET, SOCK_STREAM, 0);
        if (socket < 0)
        {
            throw std::runtime_error("Error creating socket");
        }
        int flag = 1;
        setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)); // Set TCP_NODELAY
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
    void RADPClient::sendMessage(RADPCommand command)
    {
        sendMessage(commandToString(command));
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
        int dlSizeTotal = 1;
        int dlSizeCurrent = 0;
        bool headerReceived = false;
        char buffer[(1024 * 1024)];
        std::ofstream outputFile;

        while (connected)
        {
            ssize_t bytesRead = recv(socket, buffer, sizeof(buffer), 0);
            if (bytesRead <= 0)
            {
                break;
            }

            std::string data(buffer, bytesRead);

            if (downloading && dlSizeCurrent < dlSizeTotal)
            {
                size_t headerStart = data.find("##dl##");
                size_t headerEnd = data.find("##dl##", headerStart + 6);
                if (!headerReceived && headerStart != std::string::npos && headerEnd != std::string::npos)
                {
                    std::string header = data.substr(headerStart, headerEnd - headerStart + 6);
                    dlSizeTotal = processHeader(header, &outputFile);
                    data = data.substr(headerEnd + 6);
                    headerReceived = true;
                }

                // Write data to file immediately
                if (outputFile.is_open())
                {
                    // std::cout << "Writing " << data.size() << " bytes\n";
                    outputFile.write(data.c_str(), data.size());
                    if (!outputFile)
                    {
                        std::cerr << "Error writing to file" << std::endl;
                        break;
                    }
                    dlSizeCurrent += data.size();

                    if (dlSizeTotal > 0)
                    { // Prevent division by zero
                        if (dlSizeCurrent >= dlSizeTotal)
                        {
                            printProgressBar(100, "Done");
                            std::cout << std::endl;
                            downloading = false;
                            headerReceived = false;
                            dlSizeCurrent = 0;
                            dlSizeTotal = 1;
                            outputFile.close();
                        }
                        else
                        {
                            double percentage = (static_cast<double>(dlSizeCurrent) / dlSizeTotal) * 100;
                            printProgressBar(percentage, "Downloading");
                        }
                    }
                    else
                    {
                        std::cout << "Invalid total data size!" << std::endl;
                    }
                }
                else
                {
                    std::cerr << "File not open\n";
                    break; // Break out of loop
                }
            }
            else
            {
                trimMessage(data);
                RADPCommand cmd = stringToCommand(data);
                switch (cmd)
                {
                case RADPCommand::DISCONNECTED:
                    std::cout << "Server disconnecting\n";
                    connected = false;
                    break;
                case RADPCommand::ACSDN:
                    std::cout << "Access Denied\n";
                    break;
                case RADPCommand::UKN:
                    std::cout << data << std::endl;
                    break;
                case RADPCommand::NF:
                    std::cout << "File not found\n";
                    break;
                case RADPCommand::DLST:
                    downloading = true;
                    break;
                case RADPCommand::DLF:
                    downloading = false;
                    std::cout << "\nDownload complete\n";
                    break;
                case RADPCommand::ERR:
                    std::cout << "Error\n";
                    break;
                case RADPCommand::END:
                    break;
                default:
                    std::cout << "Unhandled command\n";
                    break;
                }
            }
        }

        if (outputFile.is_open())
        {
            outputFile.close();
        }
        connected = false;
        std::cout << "Connection closed\n";
    }

    long RADPClient::processHeader(const std::string &header, std::ofstream *outputFile)
    {
        std::istringstream headerStream(header);
        std::string token;
        std::vector<std::string> tokens;

        while (headerStream >> token)
        {
            tokens.push_back(token);
        }

        if (tokens.size() >= 4 && tokens[0] == "##dl##" && tokens[tokens.size() - 1] == "##dl##")
        {
            try
            {
                unsigned long fileSize = std::stoul(tokens[1]);
                std::string fileName = tokens[2];
                std::cout << "DL " << fileName << ", " << fileSize << " bytes\n";
                outputFile->open(fileName + ".radpdl", std::ios::binary);
                if (!outputFile->is_open())
                {
                    std::cerr << "Failed to open file: " << fileName << std::endl;
                    return -1;
                }
                std::cout << "Saving to: " << fileName << ".radpdl\n";
                return fileSize;
            }
            catch (const std::invalid_argument &e)
            {
                std::cerr << "Invalid argument in header: " << e.what() << std::endl;
            }
            catch (const std::out_of_range &e)
            {
                std::cerr << "Out of range in header: " << e.what() << std::endl;
            }
            return -1;
        }
        else
        {
            std::cerr << "Invalid header format" << std::endl;
        }
        return -1;
    }
    void RADPClient::disconnect()
    {
        connected = false;
        sendMessage("EXIT\n");
        close(socket);
    }
    void RADPClient::downloadFile(const std::string &filename)
    {
        sendMessage("DL " + filename + "\n");
        sleep(1); // Wait for server to prepare for download
        waitForDownload();
    }
}