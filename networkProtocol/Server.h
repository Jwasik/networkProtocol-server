#pragma once
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <memory>
#include <vector>
#include <SFML/Network.hpp>
#include <bitset>

class Server
{
private:

	

	struct Comunicate//struktura komunikatu
	{
		UINT8 operation;//3b
		UINT8 answer;//3b
		uint16_t messageId;//16b
		uint32_t sessionId;//32b
		uint32_t datasize;//32b
		std::vector<UINT8> data;
	};
	void print(Comunicate);//metoda wypisuj�ca zawarto�� komunikatu na okno konsoli

	struct Client//struktura klienta
	{
		std::string clientName;
		sf::IpAddress clientIP;
		unsigned short clientPort;
		unsigned int sessionId;
		bool ready = 0;
		bool invited = 0;
	};
	struct Message//struktura wiadomo�ci w historii wiadomo�ci
	{
		Comunicate comunicate;
		int16_t messageId;
		bool ack;
		std::shared_ptr<Client> sender;
		std::shared_ptr<Client> receiver;
	};

	friend sf::Packet& operator<<(sf::Packet&, Server::Comunicate&);
	friend void operator>>(sf::Packet&, Server::Comunicate&);
	std::vector<UINT8> toUINTtab(std::string);//metoda zamieniaj�ca string na wektor <UINT8>

	std::shared_ptr<Client> clients[2];//tablica klient�w
	std::vector<Message> messageHistory;//historia wiadomo�ci
	uint16_t messageId = 16384;//pocz�tkowa warto�� id dla wiadomo�ci serwera
	unsigned int countClients();//metoda zliczaj�ca ilo�� pod��czonych klient�w

	sf::UdpSocket udpSocket;//g��wny socket komunikacyjny

	void sendToEveryone(Comunicate);//procedura wysy�aj�ca komunikat do wszystkich klient�w
	void sendTo(Comunicate,bool, std::shared_ptr<Client>);//procedura wysy�aj�ca komunikat do konkretnego klienta
	void retransmit(Comunicate,bool, std::shared_ptr<Client>);//procedura przesy�aj�ca komunikat do konkretnego klienta bez zmiany id wiadomo�ci

public:
	Server();
	void run();//g��wna p�tla programu
};

