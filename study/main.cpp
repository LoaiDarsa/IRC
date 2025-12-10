#include "Server.hpp"
#include <iostream>
#include <cstdlib>   // for std::atoi
#include <exception>

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
        return 1;
    }

    // Parse port number
    int port = std::atoi(argv[1]);
    if (port <= 0 || port > 65535)
    {
        std::cerr << "Error: Invalid port number" << std::endl;
        return 1;
    }

    std::string password = argv[2];

    try
    {
        Server server(port, password);
        server.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
