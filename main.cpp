#include <iostream>
#include <thread>
#include <atomic>
#include <termios.h>
#include <unistd.h>
#include <radp/radp.h>
bool running = true;
ghillie575::RADPServer* server = nullptr;
char getChar()
{
    struct termios oldt, newt;
    char ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

void listenForExit()
{
    while (running)
    {
        if (getChar() == 'q')
        {
            std::cout << "\nExiting...\n";
            server->shutdown();
            running = false;
        }
    }
}
int main(int, char **)
{   std::cout << "Hello, from libradp!\n";
    std::thread exitThread(listenForExit); // Start a thread to listen for 'q' key press
    server = new ghillie575::RADPServer(5172);
    
    ghillie575::setRadpServerDir(".");
    server->start();
    exitThread.join();
}
