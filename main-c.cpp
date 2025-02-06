#include <iostream>
#include <radp/radp.h>

int main(int, char **)
{
    std::cout << "Hello, from libradp!\n";
    ghillie575::RADPClient client("127.0.0.1", 5172);
    client.listFiles();
    sleep(1);
    client.getFileInfo("random.data");
    sleep(1);
    client.downloadFile("random.data");
    while (client.connected)
    {
    }
    return 0;
}