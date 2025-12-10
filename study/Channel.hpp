#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <set>

class Client;

class Channel {
	private:
		std::string name;
		std::string topic;

		std::set <Client> members;
		std::set<Client> operators;

	public:
		Channel(const std::string& name);

		const std::string &getName() const;
		const std::string &getTopic() const;
		void setTopic(const std::string &newTopic);

		void addMember(Client client);
		void removeMember(Client client);
		bool hasMember(Client client) const;
		const std::set<Client> &getMembers() const;

		void addOperator(Client client);
		void removeOperator(Client client);
		bool isOperator(Client client) const;

		bool empty() const;
};

#endif