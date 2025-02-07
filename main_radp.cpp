#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <radp/radp.h>

namespace fs = std::filesystem;

void printUsage(const std::string &programName)
{
    std::cout << "Usage: " << programName << " <server> <port> <target_dir> <file1> [<file2> ... <fileN>]\n";
}

int main(int argc, char *argv[])
{
    if (argc < 5)
    {
        printUsage(argv[0]);
        return 1;
    }

    std::string serverAddress = argv[1];
    int port = std::stoi(argv[2]);
    std::string targetDir = argv[3];

    // Ensure the target directory exists
    if (!fs::exists(targetDir))
    {
        std::cerr << "Target directory does not exist: " << targetDir << std::endl;
        return 1;
    }

    std::vector<std::string> filesToDownload;
    for (int i = 4; i < argc; ++i)
    {
        filesToDownload.push_back(argv[i]);
    }

    try
    {
        ghillie575::RADPClient client(serverAddress, port);

        for (const auto &file : filesToDownload)
        {
            std::cout << "Downloading file: " << file << std::endl;
            client.downloadFile(file);

            // Move the downloaded file to the target directory
            std::string downloadedFile = file + ".radpdl";
            fs::path targetPath = fs::path(targetDir) / fs::path(file);
            std::cout << "File saved to: " << targetPath << std::endl;
        }

        client.disconnect();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}