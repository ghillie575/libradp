#include <iostream>
#include <radp/radp.h>

int main(int, char **)
{
    std::cout << "Hello, from libradp!\n";
    ghillie575::RADPClient client("127.0.0.1", 5171);
    client.downloadFile("CMakeCache.txt ");
    while (client.connected)
    {
    }
    return 0;
}