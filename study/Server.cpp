#include "Server.hpp"
#include "Client.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <cctype>
#include <utility>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <netinet/in.h>
#include <sys/socket.h>

Server::Server(int port, const std::string &password)
    : port(port), serverFd(-1), password(password) {
	setUpSocket();
}

Server::~Server() {
	close(serverFd);
}

void Server::setUpSocket() {
	// Create the listening socket.
	serverFd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverFd < 0)
		throw std::runtime_error("socket() failed");

    //without this,when restarting the server will give me port already in use
	int yes = 1;
	if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
		throw std::runtime_error("setsockopt() failed");

    //creating an addr where the server will live
	sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET; //IPv4
	addr.sin_addr.s_addr = INADDR_ANY; //any IP 3ndo yeha l machine
	addr.sin_port = htons(port); //port in network format

    //binding the server add to the socket
	if (bind(serverFd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
		throw std::runtime_error("bind() failed");

    //making the socket to listen for events on this port
	if (listen(serverFd, SOMAXCONN) < 0)
		throw std::runtime_error("listen() failed");
}

//this function is called whenever a new client connects to the server
void Server::handleNewConnection()
{
	//when a client tries to connect to my server, OS bethoto bel pending queue for my listening socket
	//accept bte5ud one pending connection and creates a new socket for that specific client
	//bte5la2 a new connection laen li ma3e is only used for listening not to talk to clients directly
	int clientFd = accept(serverFd, NULL, NULL);
	if (clientFd < 0)
		return ;
	//file control, system used to control file descriptors
	//in servers, sockets are by default blocking: meaning
	//recv(fd) will freeze your entire program if no data is available
	// send(fd) will freeze if the output buffer is full
	//O_NONBLOCK means do not block
    //baleha recv would freeze my program
	fcntl(clientFd, F_SETFL, O_NONBLOCK);

	std::cout << "[NEW CLIENT] fd=" << clientFd << std::endl;
	clients.insert(std::make_pair(clientFd, Client(clientFd)));

	struct pollfd newPoll;
    newPoll.fd = clientFd;
    newPoll.events = POLLIN;
    newPoll.revents = 0;

    pollfds.push_back(newPoll);

    std::cout << "New client connected: fd = " << clientFd << std::endl;
}

void Server::run()
{
	struct pollfd pfd;
	pfd.fd = serverFd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	pollfds.push_back(pfd);

	while(true)
	{
		//waits for an event to occur
        //this blocks until new client connects, client sends data, client disconnects
		int ret = poll(&pollfds[0], pollfds.size(), -1);

		if (ret < 0)
		{
			std::cerr << "poll() failed" << std::endl;
            continue;
		}

		for(size_t i = 0;i < pollfds.size(); i++)
		{
            //hay ma3neta this socket has data to read
			if (pollfds[i].revents & POLLIN)
			{
				if (pollfds[i].fd == serverFd)
					handleNewConnection();
				else
					handleClientData(pollfds[i].fd);
			}
		}
	}
}	

//index hwwe l fd of the client
void Server::handleClientData(int fd)
{
    std::map<int, Client>::iterator it = clients.find(fd);
    if (it == clients.end()) {
        return;
    }

	char buf[512];
	//reads data from client into the buffer. TCP equivalent of read()
	int bytes = recv(fd, buf, sizeof(buf) - 1, 0);

	if (bytes <= 0)
	{
		removeClient(fd);
		return;
	}
	//TCP does NOT add a null terminator (\0), but a C-string must end with \0.
	buf[bytes] = '\0';
    //append to the client's buffer
	it->second.appendToBuffer(std::string(buf));
	std::string line;
	while (!(line = it->second.extractLine()).empty())
	{
		std::cout << "[RECV from " << fd << "] " << line << std::endl;
        handleCommand(it->second, line);
	}
}

void Server::handleCommand(Client &client, const std::string &line)
{
    std::stringstream ss(line);
    std::string cmd;
    ss >> cmd;

    // Uppercase command
    for (size_t i = 0; i < cmd.size(); i++)
        cmd[i] = std::toupper(cmd[i]);

    if (cmd == "PASS")
        handlePass(client, line);

    else if (cmd == "NICK")
        handleNick(client, line);

    else if (cmd == "USER")
        handleUser(client, line);

    else if (cmd == "JOIN")
        handleJoin(client, line);

    else if (cmd == "PART")
        handleJoin(client, line);

    else if (cmd == "PRIVMSG")
        handleJoin(client, line);

    else if (!client.isRegistered())
    {
        // client not ready
        std::string msg = ":localhost 451 * :You have not registered\r\n";
        send(client.getFd(), msg.c_str(), msg.size(), 0);
    }

    // once registered → allow other commands later (JOIN, PRIVMSG...)
}

void Server::removeClient(int fd)
{
    std::map<int, Client>::iterator clientIt = clients.find(fd);
    if (clientIt != clients.end())
    {
        Client *clientPtr = &clientIt->second;

        for (std::map<std::string, Channel>::iterator chanIt = channels.begin();
             chanIt != channels.end(); )
        {
            Channel &channel = chanIt->second;
            const bool wasMember = channel.hasMember(*clientPtr);
            const bool wasOperator = wasMember && channel.isOperator(*clientPtr);

            if (wasMember)
            {
                channel.removeMember(*clientPtr);
                channel.removeOperator(*clientPtr);

                if (wasOperator && !channel.getMembers().empty())
                {
                    Client *newOp = *channel.getMembers().begin();
                    channel.addOperator(*newOp);

                    std::string modeMsg = ":localhost MODE #" + channel.getName() +
                                          " +o " + newOp->getNickname() + "\r\n";

                    for (std::set<Client*, ClientPtrLess>::iterator it = channel.getMembers().begin();
                         it != channel.getMembers().end(); ++it)
                    {
                        send((*it)->getFd(), modeMsg.c_str(), modeMsg.size(), 0);
                    }
                }

                if (channel.empty())
                {
                    std::cout << "[CHANNEL] Deleted #" << channel.getName() << std::endl;
                    channels.erase(chanIt++);
                    continue;
                }
            }
            ++chanIt;
        }
    }

    std::cout << "[DISCONNECT] fd=" << fd << std::endl;
    close(fd);
    clients.erase(fd);

    for (std::vector<pollfd>::iterator it = pollfds.begin(); it != pollfds.end(); ++it)
    {
        if (it->fd == fd)
        {
            pollfds.erase(it);
            break;
        }
    }
}

//checks:
//Did the client already send PASS?
//Did they include a password?
//Is the password correct?
//if wrong->disconnenct
void Server::handlePass(Client &client, const std::string &line)
{
    if (client.isPassGiven())
    {
        std::string msg = ":localhost 462 PASS :You may not reregister\r\n";
        send(client.getFd(), msg.c_str(), msg.size(), 0);
        return;
    }

    std::stringstream ss(line);
    std::string cmd, pass;
    ss >> cmd >> pass;

    if (pass.empty())
    {
        std::string msg = ":localhost 461 PASS :Not enough parameters\r\n";
        send(client.getFd(), msg.c_str(), msg.size(), 0);
        return;
    }

    if (pass != this->password)
    {
        std::string msg = "ERROR :Invalid password\r\n";
        send(client.getFd(), msg.c_str(), msg.size(), 0);
        removeClient(client.getFd());
        return;
    }

    client.setPassGiven(true);
}
/* Checks:

Did user provide a nickname?

Is nickname unique?

Then sets nickname.*/
void Server::handleNick(Client &client, const std::string &line)
{
    std::stringstream ss(line);
    std::string cmd, nickname;
    ss >> cmd >> nickname;

    if (nickname.empty())
    {
        std::string msg = ":localhost 431 * :No nickname given\r\n";
        send(client.getFd(), msg.c_str(), msg.size(), 0);
        return;
    }

    // check nickname uniqueness
    for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
    {
        if (it->second.getNickname() == nickname)
        {
            std::string msg = ":localhost 433 * " + nickname + " :Nickname already in use\r\n";
            send(client.getFd(), msg.c_str(), msg.size(), 0);
            return;
        }
    }

    client.setNickname(nickname);
    client.setNickGiven(true);

    if (client.isReady())
        sendWelcome(client);
}


/*Did user provide username?

Did user already send USER?*/
void Server::handleUser(Client &client, const std::string &line)
{
    if (client.isUserGiven())
    {
        std::string msg = ":localhost 462 USER :You may not reregister\r\n";
        send(client.getFd(), msg.c_str(), msg.size(), 0);
        return;
    }

    std::stringstream ss(line);
    std::string cmd, username;
    ss >> cmd >> username;

    if (username.empty())
    {
        std::string msg = ":localhost 461 USER :Not enough parameters\r\n";
        send(client.getFd(), msg.c_str(), msg.size(), 0);
        return;
    }

    client.setUsername(username);
    client.setUserGiven(true);

    if (client.isReady())
        sendWelcome(client);
}

void Server::sendWelcome(Client &client)
{
    client.setRegistered(true);

    std::string nick = client.getNickname();

    std::string msg1 = ":localhost 001 " + nick + " :Welcome to the IRC server!\r\n";
    send(client.getFd(), msg1.c_str(), msg1.size(), 0);

    std::string msg2 = ":localhost 002 " + nick + " :Your host is localhost\r\n";
    send(client.getFd(), msg2.c_str(), msg2.size(), 0);
}

void Server::handleJoin(Client &client, const std::string &line)
{
    //Extract channel name
    std::stringstream ss(line);
    std::string cmd, channelName;
    //cmd="JOIN" channelName="#chat"
    ss >> cmd >> channelName;

    if (channelName.empty())
    {
        std::string msg = ":localhost 461 JOIN :Not enough parameters\r\n";
        send(client.getFd(), msg.c_str(), msg.size(), 0);
        return;
    }

    //removes leading #
    if (channelName[0] == '#')
        channelName = channelName.substr(1);

    // Create channel if it doesn't exist
    std::map<std::string, Channel>::iterator channelIt = channels.find(channelName);
    if (channelIt == channels.end())
    {
        std::pair<std::map<std::string, Channel>::iterator, bool> res =
            channels.insert(std::make_pair(channelName, Channel(channelName)));
        channelIt = res.first;
        std::cout << "[CHANNEL] Created #" << channelName << std::endl;
    }

    Channel &channel = channelIt->second;

    // Already in channel?
    if (channel.hasMember(client))
    {
        std::string msg = ":localhost 443 " + client.getNickname() +
                          " #" + channelName + " :You're already in the channel\r\n";
        send(client.getFd(), msg.c_str(), msg.size(), 0);
        return;
    }

    //Add client as member
    channel.addMember(client);

    // If first user → make operator
    if (channel.getMembers().size() == 1)
        channel.addOperator(client);

    std::string joinMsg = ":" + client.getNickname() + " JOIN #" + channelName + "\r\n";

    //broadcast the join message to everyone in the channel
    for (std::set<Client*, ClientPtrLess>::iterator it = channel.getMembers().begin();
         it != channel.getMembers().end(); ++it)
    {
        send((*it)->getFd(), joinMsg.c_str(), joinMsg.size(), 0);
    }

    // Send topic or "no topic"
    if (channel.getTopic().empty())
    {
        std::string noTopic = ":localhost 331 " + client.getNickname() +
                              " #" + channelName + " :No topic is set\r\n";
        send(client.getFd(), noTopic.c_str(), noTopic.size(), 0);
    }
    else
    {
        std::string topic = ":localhost 332 " + client.getNickname() +
                            " #" + channelName + " :" + channel.getTopic() + "\r\n";
        send(client.getFd(), topic.c_str(), topic.size(), 0);
    }

    // Send NAMES list (353 + 366)
    std::string names = "";
    for (std::set<Client*, ClientPtrLess>::iterator it = channel.getMembers().begin();
         it != channel.getMembers().end(); ++it)
    {
        if (channel.isOperator(**it))
            names += "@" + (*it)->getNickname() + " ";
        else
            names += (*it)->getNickname() + " ";
    }

    std::string namesReply = ":localhost 353 " + client.getNickname() +
                             " = #" + channelName + " :" + names + "\r\n";
    send(client.getFd(), namesReply.c_str(), namesReply.size(), 0);

    std::string endNames = ":localhost 366 " + client.getNickname() +
                           " #" + channelName + " :End of NAMES list\r\n";
    send(client.getFd(), endNames.c_str(), endNames.size(), 0);
}

void Server::handlePart(Client &client, const std::string& line)
{
    std::stringstream ss(line);
    std::string cmd, channelName;
    ss >> cmd >> channelName;

    if (channelName.empty())
    {
        std::string msg = ":localhost 461 PART :Not enough parameters\r\n";
        send(client.getFd(), msg.c_str(), msg.size(), 0);
        return; 
    }

    // Normalize name: strip leading '#'
    if (!channelName.empty() && channelName[0] == '#')
        channelName = channelName.substr(1);

    std::map<std::string, Channel>::iterator chanIt = channels.find(channelName);
    if (chanIt == channels.end())
    {
        std::string msg = ":localhost 403 " + client.getNickname() + " #" +
                          channelName + " :No such channel\r\n";
        send(client.getFd(), msg.c_str(), msg.size(), 0);
        return;
    }

    Channel &channel = chanIt->second;

    if (!channel.hasMember(client))
    {
        std::string msg = ":localhost 442 " + client.getNickname() + " #" +
                          channelName + " :You're not on that channel\r\n";
        send(client.getFd(), msg.c_str(), msg.size(), 0);
        return;
    }

    std::string partMsg = ":" + client.getNickname() + " PART #" + channelName + "\r\n";

    for (std::set<Client*, ClientPtrLess>::iterator it = channel.getMembers().begin();
         it != channel.getMembers().end(); ++it)
    {
        send((*it)->getFd(), partMsg.c_str(), partMsg.size(), 0);
    }

    const bool wasOperator = channel.isOperator(client);
    channel.removeMember(client);
    channel.removeOperator(client);

    if (!channel.getMembers().empty() && wasOperator)
    {
        Client *newOp = *(channel.getMembers().begin());
        channel.addOperator(*newOp);

        std::string modeMsg = ":localhost MODE #" + channelName +
                              " +o " + newOp->getNickname() + "\r\n";

        for (std::set<Client*, ClientPtrLess>::iterator it = channel.getMembers().begin();
             it != channel.getMembers().end(); ++it)
        {
            send((*it)->getFd(), modeMsg.c_str(), modeMsg.size(), 0);
        }
    }

    if (channel.empty())
    {
        channels.erase(channelName);
        std::cout << "[CHANNEL] Deleted #" << channelName << std::endl;
    }
}
