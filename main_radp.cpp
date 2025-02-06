#include <radp/radp.h>
#include <iostream>
#include <string>

void printUsage(const std::string &programName)
{
    std::cout << "Usage: " << programName << " <server_address> <port> <command> [<args>]\n";
    std::cout << "Commands:\n";
    std::cout << "  list                     List files\n";
    std::cout << "  download <filename>      Download file\n";
    std::cout << "  info <filename>          Get file info\n";
}

void executeCommand(ghillie575::RADPClient &client, const std::string &command, const std::string &arg = "")
{
    if (command == "list")
    {
        client.listFiles();
    }
    else if (command == "download")
    {
        client.downloadFile(arg);
    }
    else if (command == "info")
    {
        client.getFileInfo(arg);
    }
    else
    {
        std::cerr << "Error: Unknown command\n";
    }
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        printUsage(argv[0]);
        return 1;
    }

    std::string serverAddress = argv[1];
    int port = std::stoi(argv[2]);
    std::string command = argv[3];
    std::string arg = (argc > 4 && argv[4][0] != '-') ? argv[4] : "";
    try
    {
        ghillie575::RADPClient client(serverAddress, port);

        executeCommand(client, command, arg);
        std::string input;
        while (true)
        {
            sleep(1);
            std::cout << "Enter command (list, download <filename>, info <filename>, exit): ";
            std::getline(std::cin, input);
            if (input == "exit")
            {
                client.disconnect();
                break;
            }

            std::istringstream iss(input);
            std::string cmd, argument;
            iss >> cmd;
            iss >> argument;

            executeCommand(client, cmd, argument);

            // Allow some time for the client to process commands
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}