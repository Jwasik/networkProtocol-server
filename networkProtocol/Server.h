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
	void print(Comunicate);//metoda wypisuj¹ca zawartoœæ komunikatu na okno konsoli

	struct Client//struktura klienta
	{
		std::string clientName;
		sf::IpAddress clientIP;
		unsigned short clientPort;
		unsigned int sessionId;
		bool ready = 0;
		bool invited = 0;
	};
	struct Message//struktura wiadomoœci w historii wiadomoœci
	{
		Comunicate comunicate;
		int16_t messageId;
		bool ack;
		std::shared_ptr<Client> sender;
		std::shared_ptr<Client> receiver;
	};

	friend sf::Packet& operator<<(sf::Packet&, Server::Comunicate&);
	friend void operator>>(sf::Packet&, Server::Comunicate&);
	std::vector<UINT8> toUINTtab(std::string);//metoda zamieniaj¹ca string na wektor <UINT8>

	std::shared_ptr<Client> clients[2];//tablica klientów
	std::vector<Message> messageHistory;//historia wiadomoœci
	uint16_t messageId = 16384;//pocz¹tkowa wartoœæ id dla wiadomoœci serwera
	unsigned int countClients();//metoda zliczaj¹ca iloœæ pod³¹czonych klientów

	sf::UdpSocket udpSocket;//g³ówny socket komunikacyjny

	void sendToEveryone(Comunicate);//procedura wysy³aj¹ca komunikat do wszystkich klientów
	void sendTo(Comunicate,bool, std::shared_ptr<Client>);//procedura wysy³aj¹ca komunikat do konkretnego klienta
	void retransmit(Comunicate,bool, std::shared_ptr<Client>);//procedura przesy³aj¹ca komunikat do konkretnego klienta bez zmiany id wiadomoœci

public:
	Server();
	void run();//g³ówna pêtla programu
};

