#pragma once
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <memory>
#include <vector>
#include <SFML/Network.hpp>

class Server
{
private:

	

	struct Comunicate
	{
		UINT8 operation;//3b
		UINT8 answer;//3b
		int16_t messageId;//16b
		uint32_t sessionId;//32b
		uint32_t datasize;//32b
		std::vector<UINT8> data;
	};
	void print(Comunicate);
	struct Client
	{
		std::string clientName;
		sf::IpAddress clientIP;
		unsigned short clientPort;
		unsigned int sessionId;
		bool ready = 0;
		bool invited = 0;
	};
	struct Message
	{
		Comunicate comunicate;
		int16_t messageId;
		bool ack;
		std::shared_ptr<Client> sender;
		std::shared_ptr<Client> receiver;
	};

	friend sf::Packet& operator<<(sf::Packet&, Server::Comunicate&);
	friend void operator>>(sf::Packet&, Server::Comunicate&);
	std::vector<UINT8> toUINTtab(std::string);

	std::shared_ptr<Client> clients[2];
	std::vector<Message> messageHistory;
	int16_t messageId = -1;

	sf::UdpSocket udpSocket;

	void sendToEveryone(Comunicate);
	void sendTo(Comunicate,bool, std::shared_ptr<Client>);
	void sendTo(Comunicate,std::shared_ptr<Client>&, std::shared_ptr<Client>);

	std::string prepareClientsList();

public:
	Server();
	void run();

};

