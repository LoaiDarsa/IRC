#include "Channel.hpp"
#include "Client.hpp"

Channel::Channel(const std::string& name): name(name), topic(""){}

const std::string &Channel::getName() const {
    return name;
}

const std::string &Channel::getTopic() const {
    return topic;
}

void Channel::setTopic(const std::string &newTopic) {
    topic = newTopic;
}

void Channel::addMember(Client client) {
    members.insert(client);
}

void Channel::removeMember(Client client) {
	members.erase(client);
    operators.erase(client); 
}

bool Channel::hasMember(Client client) const {
	return members.find(client) != members.end();
}

const std::set<Client> &Channel::getMembers() const {
    return members;
}

void Channel::addOperator(Client client) {
    operators.insert(client);
    members.insert(client);
}

void Channel::removeOperator(Client client) {
    operators.erase(client);
}

bool Channel::isOperator(Client client) const {
    return operators.find(client) != operators.end();
}

bool Channel::empty() const {
    return members.empty();
}