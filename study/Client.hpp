#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client {
private:
    int fd;
    std::string buffer;
    std::string nickname;
    std::string username;

    bool passGiven;
    bool nickGiven;
    bool userGiven;
    bool registered;

public:
    explicit Client(int fd);

    int getFd() const;

    void appendToBuffer(const std::string &data);

    std::string extractLine();

    bool isPassGiven() const;
    bool isNickGiven() const;
    bool isUserGiven() const;

    void setPassGiven(bool v);
    void setNickGiven(bool v);
    void setUserGiven(bool v);

    void setNickname(const std::string& n);
    void setUsername(const std::string& u);

    std::string getNickname() const;
    std::string getUsername() const;

    bool operator<(const Client &other) const;

    bool isReady() const;

    void setRegistered(bool v);
    bool isRegistered() const;
};

#endif
