#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include <poll.h>
#include "Client.hpp"

class Server {
private:
    int port;
    int serverFd;
    std::string password;

    std::vector<struct pollfd> pollfds;
    std::map<int, Client> clients;

public:
    Server(int port, const std::string &password);
    ~Server();

    void run();

private:
    // Core network functions
    void setUpSocket();
    void handleNewConnection();
    void handleClientData(int fd);
    void removeClient(int fd);

    // IRC command dispatcher
    void handleCommand(Client &client, const std::string &line);

    // Authentication handlers
    void handlePass(Client &client, const std::string &line);
    void handleNick(Client &client, const std::string &line);
    void handleUser(Client &client, const std::string &line);

    // Welcome message after full registration
    void sendWelcome(Client &client);
};

#endif // SERVER_HPP
