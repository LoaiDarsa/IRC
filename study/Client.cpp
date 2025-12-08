#include "Client.hpp"

Client::Client(int fd)
    : fd(fd), passGiven(false), nickGiven(false), userGiven(false), registered(false) {}

int Client::getFd() const { return fd; }

void Client::appendToBuffer(const std::string &data) { buffer += data; }

std::string Client::extractLine()
{
    size_t pos = buffer.find('\n');
    if (pos == std::string::npos)
        return "";

    std::string line = buffer.substr(0, pos);
    buffer.erase(0, pos + 1);

    if (!line.empty() && line.back() == '\r')
        line.pop_back();

    return line;
}

bool Client::isPassGiven() const { return passGiven; }
bool Client::isNickGiven() const { return nickGiven; }
bool Client::isUserGiven() const { return userGiven; }

void Client::setPassGiven(bool v) { passGiven = v; }
void Client::setNickGiven(bool v) { nickGiven = v; }
void Client::setUserGiven(bool v) { userGiven = v; }

void Client::setNickname(const std::string &n) { nickname = n; }
void Client::setUsername(const std::string &u) { username = u; }

std::string Client::getNickname() const { return nickname; }
std::string Client::getUsername() const { return username; }

bool Client::isReady() const
{
    return passGiven && nickGiven && userGiven && !registered;
}

void Client::setRegistered(bool v) { registered = v; }
bool Client::isRegistered() const { return registered; }
