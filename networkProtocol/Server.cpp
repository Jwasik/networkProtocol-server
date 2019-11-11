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

void Server::sendTo(Comunicate comunicate, bool receiver, std::shared_ptr<Client> sender = nullptr)
{//u¿ywana do wysy³ania komunikatu serwera
	if (clients[receiver] == nullptr)return;
	Message message{ comunicate ,messageId ,0,sender,clients[receiver] };
	messageHistory.push_back(message);

	sf::Packet packet;
	comunicate.sessionId = clients[receiver]->sessionId;
	comunicate.datasize = comunicate.data.size();
	packet << comunicate;
	this->udpSocket.send(packet, clients[receiver]->clientIP, clients[receiver]->clientPort);
	this->messageId--;
}

void Server::sendTo(Comunicate comunicate, std::shared_ptr<Client>& client, std::shared_ptr<Client> sender = nullptr)
{
	Message message{ comunicate ,messageId ,0,sender,client };
	messageHistory.push_back(message);

	sf::Packet packet;
	comunicate.sessionId = client->sessionId;
	packet << comunicate;
	this->udpSocket.send(packet, client->clientIP, client->clientPort);
	this->messageId--;
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

		if (udpSocket.receive(receivedPacket, receivedIP, receivedPort) == sf::Socket::Done)
		{
			bool sender;
			Comunicate receivedComunicate;
			receivedPacket >> receivedComunicate;
			if (clients[0] != nullptr && receivedIP == clients[0]->clientIP && receivedPort == clients[0]->clientPort)
			{
				sender = 0;
			}
			if (clients[1] != nullptr && receivedIP == clients[1]->clientIP && receivedPort == clients[1]->clientPort)
			{
				sender = 1;
			}
			else sender = 0;

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
							
							Comunicate answerComunicate = { 1,6,this->messageId,sessionId,0 ,std::vector<UINT8>() };
							sf::Packet answerPacket;
							answerPacket << answerComunicate;
							this->udpSocket.send(answerPacket,receivedIP,receivedPort);

							continue;
						}
					}
					Comunicate answerComunicate = { 1,7,this->messageId,sessionId,0 ,std::vector<UINT8>() };

					this->sendTo(answerComunicate, newCLient);

					std::vector<UINT8> clientList = toUINTtab(this->prepareClientsList());
					answerComunicate = Comunicate{ 7,7,messageId,0,this->prepareClientsList().length(),clientList };
					if (clients[sender] != nullptr)this->sendTo(answerComunicate, sender);
					if (clients[!sender] != nullptr)this->sendTo(answerComunicate, !sender);
					std::cout << "Client joined succesfully" << std::endl;

					messageId--;
				}
				if (receivedComunicate.answer == 3)//roz³¹cz
				{
					//roz³¹czenie
					Comunicate answerComunicate{ 1,3,messageId,0,0,std::vector<UINT8>()};
					this->sendTo(answerComunicate, sender);
					clients[sender] = nullptr;

					//poinformowanie innych klientów
					answerComunicate = Comunicate{ 7,7,0,0,0,this->toUINTtab("Client left server") };
					answerComunicate = Comunicate{ 7,7,0,0,0,toUINTtab(this->prepareClientsList()) };
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
						Comunicate message{ 7,7,0,0,26,this->toUINTtab("Not found any other client") };
						this->sendTo(message, sender);
					}
					else if (clients[!sender]->invited == 1)//ju¿ zaproszony
					{
						//wys³anie b³êdu
						receivedComunicate.answer = 3;
						this->sendTo(receivedComunicate, sender);

						//wys³anie opisu b³êdu
						Comunicate message{ 7,7,0,0,15,this->toUINTtab("Already invited") };
						this->sendTo(message, sender);
					}
					else
					{
						//wys³anie potwierdzenia zaproszenia
						Comunicate ackComunicate{ 7,4,receivedComunicate.messageId,0,0,std::vector<UINT8>() };
						this->sendTo(ackComunicate, sender);

						//przekazanie zaproszenia
						this->sendTo(receivedComunicate, !sender);
						clients[!sender]->invited = 1;
						clients[sender]->ready = 1;
					}
				}
				if (receivedComunicate.answer == 1)//accept
				{
					if (clients[sender]->invited == 1)//akceptacja
					{
						receivedComunicate.operation = 7;
						receivedComunicate.answer = 3;
						sendTo(receivedComunicate, sender);

						Comunicate message{ 7,7,0,0,0,this->toUINTtab("Succesfully accepted invitation") };
						this->sendTo(message, sender);

						clients[sender]->ready = 1;

						//poinformowanie zapraszaj¹cego
						message = Comunicate{ 7,7,0,0,0,this->toUINTtab("Your invitation has been accepted") };
						this->sendTo(message, !sender);
					}
					else if (clients[sender]->ready == 1)//ju¿ jest w konwersacji
					{
						receivedComunicate.operation = 7;
						receivedComunicate.answer = 3;
						sendTo(receivedComunicate, sender);

						Comunicate message{ 7,7,0,0,0,this->toUINTtab("You are already member of conversation") };
						this->sendTo(message, sender);
					}
					else//nie zosta³ zaproszony
					{
						receivedComunicate.operation = 7;
						receivedComunicate.answer = 3;
						sendTo(receivedComunicate, sender);

						Comunicate message{ 7,7,0,0,0,this->toUINTtab("You are not invited to any conversation") };
						this->sendTo(message, sender);
					}
				}
				if (receivedComunicate.answer == 2)//deny
				{
					if (clients[sender]->invited == 1)//odmowa
					{
						receivedComunicate.operation = 7;
						receivedComunicate.answer = 3;
						sendTo(receivedComunicate, sender);

						Comunicate message{ 7,7,0,0,0,this->toUINTtab("Succesfully denied invitation") };
						this->sendTo(message, sender);

						clients[sender]->invited = 0;

						//poinformowanie zapraszaj¹cego
						message = Comunicate{ 7,7,0,0,0,this->toUINTtab("Your invitation has been denied") };
						this->sendTo(message, !sender);
					}
					else//nie zosta³ zaproszony
					{
						receivedComunicate.operation = 7;
						receivedComunicate.answer = 3;
						sendTo(receivedComunicate, sender);

						Comunicate message{ 7,7,0,0,0,this->toUINTtab("You are not invited to any conversation") };
						this->sendTo(message, sender);
					}
				}
			}

			if (receivedComunicate.operation == 7)//msg
			{
				if (receivedComunicate.answer == 0)//zwyk³a wiadomoœæ
				{
					std::cout << "Got msg from " << sender << " to " << !sender << std::endl;

					//wiadomoœæ zwrotna do nadawcy (ACK)
					Comunicate ackComunicate = receivedComunicate;
					ackComunicate.answer = 3;
					ackComunicate.datasize = 0;
					ackComunicate.data = std::vector<UINT8>();
					sf::Packet ackPacket;
					ackPacket << ackComunicate;
					this->sendTo(ackComunicate, sender);

					//przes³anie wiadomoœci do odbiorcy
					if (clients[!sender] != nullptr && clients[!sender]->ready == 1)
					{
						sf::Packet msgPacket;
						msgPacket << receivedComunicate;
						this->sendTo(receivedComunicate, !sender);
					}
				}
				if (receivedComunicate.answer == 3)//ack msg
				{
					//Przes³aæ dalej do drugiego klienta
					this->sendTo(receivedComunicate, !sender);
				}
			}

		}
	}
}

sf::Packet& operator<<(sf::Packet& packet, Server::Comunicate& comunicate)
{
	packet << comunicate.operation << comunicate.answer << comunicate.messageId << comunicate.sessionId << comunicate.datasize;
	for (auto& letter : comunicate.data)
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