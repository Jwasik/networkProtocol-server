#include "Server.h"

unsigned int Server::countClients()
{
	unsigned int counter = 0;
	if (clients[0] != nullptr)counter++;
	if (clients[1] != nullptr)counter++;
	return counter;
}

void Server::sendToEveryone(Comunicate comunicate)
{
	sf::Packet packet;
	if (clients[0] != nullptr)
	{
		Message message{ comunicate ,messageId ,0,nullptr,clients[0] };
		messageHistory.push_back(message);
		comunicate.messageId = this->messageId;
		this->messageId++;
		comunicate.sessionId = clients[0]->sessionId;
		packet << comunicate;
		this->udpSocket.send(packet, clients[0]->clientIP, clients[0]->clientPort);
		packet.clear();
	}
	if (clients[1] != nullptr)
	{
		Message message{ comunicate ,messageId ,0,nullptr,clients[1] };
		messageHistory.push_back(message);
		comunicate.messageId = this->messageId;
		this->messageId++;
		comunicate.sessionId = clients[1]->sessionId;
		packet << comunicate;
		packet.clear();
		this->udpSocket.send(packet, clients[1]->clientIP, clients[1]->clientPort);
	}
}

void Server::sendTo(Comunicate comunicate, bool receiver, std::shared_ptr<Client> sender = nullptr)
{//u¿ywana do wysy³ania komunikatu serwera
	if (clients[receiver] == nullptr)return;
	Message message{ comunicate ,this->messageId ,0,sender,clients[receiver] };
	messageHistory.push_back(message);

	sf::Packet packet;
	comunicate.messageId = this->messageId;
	this->messageId++;
	comunicate.sessionId = clients[receiver]->sessionId;
	comunicate.datasize = comunicate.data.size();
	packet << comunicate;
	this->udpSocket.send(packet, clients[receiver]->clientIP, clients[receiver]->clientPort);
}

void Server::retransmit(Comunicate comunicate, bool receiver, std::shared_ptr<Client> sender = nullptr)
{
	if (clients[receiver] == nullptr)return;
	Message message{ comunicate ,comunicate.messageId ,0,sender,clients[receiver] };

	sf::Packet packet;
	comunicate.sessionId = clients[receiver]->sessionId;
	comunicate.datasize = comunicate.data.size();
	packet << comunicate;
	this->udpSocket.send(packet, clients[receiver]->clientIP, clients[receiver]->clientPort);
}

void Server::sendTo(Comunicate comunicate, std::shared_ptr<Client>& client, std::shared_ptr<Client> sender = nullptr)
{
	Message message{ comunicate ,this->messageId ,0,sender,client };
	messageHistory.push_back(message);

	sf::Packet packet;
	comunicate.messageId = this->messageId;
	this->messageId++;
	comunicate.sessionId = client->sessionId;
	packet << comunicate;
	this->udpSocket.send(packet, client->clientIP, client->clientPort);
}

std::string Server::prepareClientsList()
{
	std::string str = "Error, no clients";
	if (clients[0] != nullptr && clients[1] != nullptr)
	{
		str = "There is one more client on server";
		return str;
	}
	if (clients[0] != nullptr || clients[1] != nullptr)
	{
		str = "You are the only client on server";
		return str;
	}
	return str;
}

Server::Server()
{
	udpSocket.bind(8888);
	udpSocket.setBlocking(false);
}

void Server::run()
{
	std::cout << sf::IpAddress::getLocalAddress().toString() << std::endl;
	srand(time(NULL));

	while (1)
	{
		sf::IpAddress receivedIP;
		unsigned short receivedPort;
		sf::Packet receivedPacket;
		receivedPacket.clear();

		if (udpSocket.receive(receivedPacket, receivedIP, receivedPort) == sf::Socket::Done)
		{
			bool sender;
			Comunicate receivedComunicate;
			receivedPacket >> receivedComunicate;

			std::cout << "received " << (int)receivedComunicate.operation << ' ' << (int)receivedComunicate.answer << std::endl;

			if (clients[0] != nullptr && receivedIP == clients[0]->clientIP && receivedPort == clients[0]->clientPort)
			{
				sender = 0;
			}
			if (clients[1] != nullptr && receivedIP == clients[1]->clientIP && receivedPort == clients[1]->clientPort)
			{
				sender = 1;
			}
			else sender = 0;

			if (receivedComunicate.operation == 1)
			{

			}
			else if (receivedComunicate.operation == 7 &&receivedComunicate.answer == 3 && receivedComunicate.messageId < 16384)//ack msg
			{
				//Przes³aæ dalej potwierdzenie
				std::cout << "retransmiting: " << receivedComunicate.messageId << std::endl;
				this->retransmit(receivedComunicate, !sender);
				continue;
			}
			else if(receivedComunicate.messageId < 16384)//ack message
			{
				std::cout << "sending ack: " << receivedComunicate.messageId << std::endl;
				Comunicate ackComunicate{ 7,3,receivedComunicate.messageId,clients[sender]->sessionId,0,std::vector<UINT8>() };
				sf::Packet ackPacket;
				ackPacket << ackComunicate;
				this->udpSocket.send(ackPacket, receivedIP, receivedPort);
			}

			if (receivedComunicate.operation == 1)//join
			{
				if (receivedComunicate.answer == 0)
				{
					uint32_t sessionId = rand();
					std::cout << "set new session ID to " << sessionId << std::endl;
					auto newCLient = clients[0];
					if (clients[0] == nullptr)
					{
						clients[0] = std::make_shared<Client>(Client{ "noname",receivedIP,receivedPort,sessionId,0,0 });
						newCLient = clients[0];
					}
					else
					{
						if (clients[1] == nullptr)
						{
							clients[1] = std::make_shared<Client>(Client{ "noname",receivedIP,receivedPort,sessionId,0,0 });
							newCLient = clients[1];
						}
						else
						{
							//gdy nie ma miejsca
							std::cout << "nie ma miejsca" << std::endl;
							Comunicate answerComunicate = { 1,6,this->messageId,sessionId,0 ,std::vector<UINT8>() };
							sf::Packet answerPacket;
							answerPacket << answerComunicate;
							this->udpSocket.send(answerPacket,receivedIP,receivedPort);

							continue;
						}
					}
					Comunicate answerComunicate = { 1,7,this->messageId,sessionId,0 ,std::vector<UINT8>() };

					sf::Packet answerPacket;
					answerPacket << answerComunicate;

					this->udpSocket.send(answerPacket,receivedIP,receivedPort);

					std::vector<UINT8> clientList = toUINTtab(this->prepareClientsList());

					this->messageId++;
				}
				if (receivedComunicate.answer == 3)//roz³¹cz
				{
					//roz³¹czenie
					Comunicate answerComunicate{ 1,4,messageId,0,0,std::vector<UINT8>()};
					this->sendTo(answerComunicate, sender);
					clients[sender] = nullptr;

					//poinformowanie innych klientów
					answerComunicate = Comunicate{ 3,3,0,0,0,this->toUINTtab("Client left server") };
					this->sendToEveryone(answerComunicate);
				}
				if (receivedComunicate.answer == 5)
				{
					Comunicate answerComunicate{ 3,0,messageId,0,0,std::vector<UINT8>() };
					if (countClients() == 2)answerComunicate = Comunicate{ 3,1,messageId,0,0,std::vector<UINT8>() };

					if (clients[sender] != nullptr)this->sendTo(answerComunicate, sender);
					if (clients[!sender] != nullptr)this->sendTo(answerComunicate, !sender);
				}
			}

			if (receivedComunicate.operation == 2)//invite
			{
				if (receivedComunicate.answer == 0)//invite
				{
					if (clients[!sender] == nullptr)//nie ma innego klienta
					{
						//wys³anie b³êdu
						receivedComunicate.answer = 3;
						this->sendTo(receivedComunicate, sender);

						//wys³anie opisu b³êdu
						Comunicate message{ 3,2,0,0,0,std::vector<UINT8>() };
						this->sendTo(message, sender);
					}
					else if (clients[!sender]->invited == 1)//ju¿ zaproszony
					{
						//wys³anie b³êdu
						receivedComunicate.answer = 3;
						this->sendTo(receivedComunicate, sender);

						//wys³anie opisu b³êdu
						Comunicate message{ 3,4,0,0,0,std::vector<UINT8>() };
						this->sendTo(message, sender);
					}
					else
					{
						//przekazanie zaproszenia
						this->sendTo(receivedComunicate, !sender);
						clients[!sender]->invited = 1;
						clients[sender]->ready = 0;
					}
				}
				if (receivedComunicate.answer == 1)//accept
				{
					if (clients[sender]->invited == 1)//akceptacja
					{
						clients[sender]->ready = 1;
						if(clients[!sender] != nullptr)clients[!sender]->ready = 1;

						//poinformowanie zapraszaj¹cego
						Comunicate message{ 3,5,0,0,0,std::vector<UINT8>()};
						this->sendTo(message, !sender);
					}
					else if (clients[sender]->ready == 1)//ju¿ jest w konwersacji
					{
						Comunicate message{ 3,4,0,0,0,std::vector<UINT8>() };
						this->sendTo(message, sender);
					}
					else//nie zosta³ zaproszony
					{
						Comunicate message{ 3,7,0,0,0,std::vector<UINT8>() };
						this->sendTo(message, sender);
					}
				}
				if (receivedComunicate.answer == 2)//deny
				{
					if (clients[sender]->invited == 1)//ok
					{
						clients[sender]->invited = 0;
						if(clients[!sender] != nullptr)clients[!sender]->ready = 0;

						Comunicate message{ 3,6,0,0,0,std::vector<UINT8>() };
						this->sendTo(message, !sender);
					}
					else//nie zosta³ zaproszony
					{
						Comunicate message{ 3,7,0,0,0,std::vector<UINT8>() };
						this->sendTo(message, sender);
					}
				}
			}

			if (receivedComunicate.operation == 7)//msg
			{
				if (receivedComunicate.answer == 0)//zwyk³a wiadomoœæ
				{
					std::cout << "Got msg from " << sender << " to " << !sender << std::endl;

					//przes³anie wiadomoœci do odbiorcy
					if (clients[!sender] != nullptr && clients[!sender]->ready == 1)
					{
						sf::Packet msgPacket;
						msgPacket << receivedComunicate;
						this->retransmit(receivedComunicate, !sender);
					}
				}
			}

		}
	}
}

sf::Packet& operator<<(sf::Packet& packet, Server::Comunicate& comunicate)
{
	packet.clear();
	std::string msg = "";
	msg += std::bitset< 3 >(comunicate.operation).to_string();
	msg += std::bitset< 3 >(comunicate.answer).to_string();
	msg += std::bitset< 32 >(comunicate.datasize).to_string();

	for (auto& letter : comunicate.data)
	{
		msg += std::bitset< 8 >(int(letter)).to_string();
	}

	msg += std::bitset< 32 >(comunicate.sessionId).to_string();
	msg += std::bitset< 32 >(comunicate.messageId).to_string();

	while (msg.length()%8 != 0)
	{
		msg += '0';
	}
	std::cout << "prepared " << msg << std::endl;

	UINT8 pom = 0;
	while (msg.length() > 0)
	{
		std::string pom2 = msg.substr(0,8);
		pom = std::stoi(pom2 , 0, 2);
		msg.erase(0, 8);
		packet << pom;
	}
	return packet;
}

void operator>>(sf::Packet& packet, Server::Comunicate& comunicate)
{
	std::string msg = "";
	UINT8 pom;

	while (!packet.endOfPacket())
	{
		packet >> pom;
		std::string s = std::bitset< 8 >(pom).to_string();
		msg += s;
	}
	std::cout << "received " << msg << std::endl;

	std::string pom2 = msg.substr(0, 3);
	msg.erase(0, 3);
	comunicate.operation = std::stoi(pom2,0,2);

	pom2 = msg.substr(0, 3);
	msg.erase(0, 3);
	comunicate.answer = std::stoi(pom2, 0, 2);

	pom2 = msg.substr(0, 32);
	msg.erase(0, 32);
	comunicate.datasize = std::stoi(pom2, 0, 2);

	for (int i = 0; i < comunicate.datasize; i++)
	{
		pom2 = msg.substr(0, 8);
		msg.erase(0, 8);
		UINT8 data;
		if (pom2.length() != 0) 
		{
			data = std::stoi(pom2, 0, 2);
			comunicate.data.push_back(data);
		}
	}

	pom2 = msg.substr(0, 32);
	msg.erase(0, 32);
	comunicate.sessionId = std::stoi(pom2, 0, 2);

	pom2 = msg.substr(0, 32);
	if(pom2.length() != 0)comunicate.messageId = std::stoi(pom2, 0, 2);
}

void Server::print(Comunicate com)
{
	std::cout << "opr: " << (unsigned int)com.operation << " ans: " << (unsigned int)com.answer << "  mesID: " << (int)com.messageId << " SID: " << (unsigned int)com.sessionId << " datasize: " << (unsigned int)com.datasize << std::endl;
	for (const auto &letter : com.data)
	{
		std::cout << letter << ' ';
	}
	std::cout << std::endl;
}

std::vector<UINT8> Server::toUINTtab(std::string string)
{
	std::vector<UINT8> vector;
	for (const auto& letter : string)
	{
		vector.push_back((UINT8)letter);
	}
	return vector;
}