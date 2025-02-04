#include <iostream>
#include <radp/radp.h>
int main(int, char **)
{
    std::cout << "Hello, from libradp!\n";
    ghillie575::RADPServer server(5172);
    ghillie575::setRadpServerDir("./");
    server.start();
}
