#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <set>

class Client;

struct ClientPtrLess {
	bool operator()(const Client *lhs, const Client *rhs) const;
};

class Channel {
	private:
		std::string name;
		std::string topic;

		std::set<Client*, ClientPtrLess> members;
		std::set<Client*, ClientPtrLess> operators;

	public:
		Channel(const std::string& name);

		const std::string &getName() const;
		const std::string &getTopic() const;
		void setTopic(const std::string &newTopic);

		void addMember(Client &client);
		void removeMember(Client &client);
		bool hasMember(const Client &client) const;
		const std::set<Client*, ClientPtrLess> &getMembers() const;

		void addOperator(Client &client);
		void removeOperator(Client &client);
		bool isOperator(const Client &client) const;

		bool empty() const;
};

#endif
