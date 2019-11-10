#include "Server.h"

void Server::sendToEveryone(Comunicate comunicate)
{
	sf::Packet packet;
	if (clients[0] != nullptr)
	{
		Message message{ comunicate ,messageId ,0,nullptr,clients[0] };
		messageHistory.push_back(message);
		comunicate.messageId = this->messageId;
		comunicate.sessionId = clients[0]->sessionId;
		packet << comunicate;
		this->udpSocket.send(packet, clients[0]->clientIP, clients[0]->clientPort);
		packet.clear();
		messageId--;
	}
	if (clients[1] != nullptr)
	{
		Message message{ comunicate ,messageId ,0,nullptr,clients[1] };
		messageHistory.push_back(message);
		comunicate.messageId = this->messageId;
		comunicate.sessionId = clients[1]->sessionId;
		packet << comunicate;
		packet.clear();
		this->udpSocket.send(packet, clients[1]->clientIP, clients[1]->clientPort);
		messageId--;
	}
}

void Server::sendTo(Comunicate comunicate,bool receiver, std::shared_ptr<Client> sender = nullptr)
{//u¿ywana do wysy³ania komunikatu serwera
	Message message{ comunicate ,messageId ,0,sender,clients[receiver] };
	messageHistory.push_back(message);

	sf::Packet packet;
	comunicate.sessionId = clients[receiver]->sessionId;
	packet << comunicate;
	this->udpSocket.send(packet, clients[receiver]->clientIP, clients[receiver]->clientPort);
	this->messageId--;
}

void Server::sendTo(Comunicate comunicate,std::shared_ptr<Client>& client, std::shared_ptr<Client> sender = nullptr)
{
	Message message{ comunicate ,messageId ,0,sender,client };
	messageHistory.push_back(message);

	sf::Packet packet;
	comunicate.sessionId = client->sessionId;
	packet << comunicate;
	this->udpSocket.send(packet,client->clientIP,client->clientPort);
	this->messageId--;
}

std::string Server::prepareClientsList()
{
	std::string str = "Available clients:";
	if (clients[0] != nullptr)str += " " + clients[0]->clientName;
	if (clients[1] != nullptr)str += " " + clients[1]->clientName;
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

		if (udpSocket.receive(receivedPacket, receivedIP, receivedPort) == sf::Socket::Done)
		{
			bool sender;
			Comunicate receivedComunicate;
			receivedPacket >> receivedComunicate;
			if (clients[0] != nullptr && receivedIP == clients[0]->clientIP)sender = 0;
			if (clients[1] != nullptr && receivedIP == clients[1]->clientIP)sender = 1;

			if (receivedComunicate.operation == 1)//join
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
						std::cout << "server full" << std::endl;
						continue;
					}
				}
				
				std::cout << newCLient->clientIP.toString() <<":"<<newCLient->clientPort<< std::endl;
				Comunicate answerComunicate = {1,7,this->messageId,sessionId,0 ,std::vector<UINT8>()};

				this->sendTo(answerComunicate,newCLient);

				std::vector<UINT8> clientList = toUINTtab(this->prepareClientsList());
				answerComunicate = Comunicate{7,7,messageId,0,this->prepareClientsList().length(),clientList};
				this->sendToEveryone(answerComunicate);
				std::cout << "Client joined succesfully" << std::endl;
			}

			if (receivedComunicate.operation == 7)//msg
			{
				if (receivedComunicate.answer == 0)//zwyk³a wiadomoœæ
				{
					//wiadomoœæ zwrotna do nadawcy (ACK)
					Comunicate ackComunicate = receivedComunicate;
					ackComunicate.answer = 3;
					ackComunicate.datasize = 0;
					ackComunicate.data = std::vector<UINT8>();
					sf::Packet ackPacket;
					ackPacket << ackComunicate;
					this->sendTo(ackComunicate,sender);

					//przes³anie wiadomoœci do odbiorcy
					if (clients[!sender] != nullptr)
					{
						sf::Packet msgPacket;
						msgPacket << receivedComunicate;
						this->sendTo(receivedComunicate, !sender);
					}
				}
				if (receivedComunicate.answer == 3)//ack msg
				{
					//Przes³aæ dalej do drugiego klienta
				}
			}
			
		}
	}
}

sf::Packet& operator<<(sf::Packet& packet, Server::Comunicate& comunicate)
{
	packet << comunicate.operation << comunicate.answer << comunicate.messageId << comunicate.sessionId << comunicate.datasize;
	for (auto & letter : comunicate.data)
	{
		packet << letter;
	}
	return packet;
}

void operator>>(sf::Packet& packet, Server::Comunicate& comunicate)
{
	packet >> comunicate.operation >> comunicate.answer >> comunicate.messageId >> comunicate.sessionId >> comunicate.datasize;
	UINT8 data;
	for (uint32_t i = 0; i < comunicate.datasize; i++)
	{
		packet >> data;
		comunicate.data.push_back(data);
	}
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