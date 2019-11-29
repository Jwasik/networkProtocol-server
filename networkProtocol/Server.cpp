#include "Server.h"

unsigned int Server::countClients()
{
	unsigned int counter = 0;
	if (clients[0] != nullptr)counter++;
	if (clients[1] != nullptr)counter++;
	return counter;
}

void Server::sendToEveryone(Comunicate comunicate)
{//procedura wysy³aj¹ca komunikat do wszystkich klientów
	sf::Packet packet;
	if (clients[0] != nullptr)
	{
		Message message{ comunicate ,messageId ,0,nullptr,clients[0] };//tworzy wiadomoœæ do historii wiadomoœci
		messageHistory.push_back(message);
		comunicate.messageId = this->messageId;//nadaje numer komunikatu
		this->messageId++;
		comunicate.sessionId = clients[0]->sessionId;//zmienia id sesji komunikatu na id sesji odbiorcy
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

void Server::sendTo(Comunicate comunicate, bool receiver, std::shared_ptr<Client> sender = nullptr)//receiver to numer odbiorcy. 0 lub 1
{//u¿ywana do wysy³ania komunikatu od serwera
	if (clients[receiver] == nullptr)return;
	Message message{ comunicate ,this->messageId ,0,sender,clients[receiver] };//tworzy wiadomoœæ do historii wiadomoœci
	messageHistory.push_back(message);

	sf::Packet packet;
	comunicate.messageId = this->messageId;//nadaje numer komunikatu
	this->messageId++;
	comunicate.sessionId = clients[receiver]->sessionId;//zmienia id sesji komunikatu na id sesji odbiorcy
	comunicate.datasize = comunicate.data.size();
	packet << comunicate;
	this->udpSocket.send(packet, clients[receiver]->clientIP, clients[receiver]->clientPort);
}

void Server::retransmit(Comunicate comunicate, bool receiver, std::shared_ptr<Client> sender = nullptr)//receiver to numer odbiorcy. 0 lub 1
{//procedura przesy³aj¹ca wiadomoœæ miêdzy klientami bez zmiany id wiadomoœci
	if (clients[receiver] == nullptr)return;
	Message message{ comunicate ,comunicate.messageId ,0,sender,clients[receiver] };

	sf::Packet packet;
	comunicate.sessionId = clients[receiver]->sessionId;//zmienia id sesji komunikatu na id sesji odbiorcy
	comunicate.datasize = comunicate.data.size();
	packet << comunicate;
	this->udpSocket.send(packet, clients[receiver]->clientIP, clients[receiver]->clientPort);
}

Server::Server()
{
	udpSocket.bind(8888);
	udpSocket.setBlocking(false);
}

void Server::run()//g³ówna procedura serwera
{//dzia³anie serwera opiera siê na odbieraniu wiadomoœci i podejmowaniu do nich stosownych dzia³añ
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

			//ustalamy nadawcê wiadomoœci, gdy nadawca nie jest klientem ustawiamy go na 0
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
				//Przesy³amy potwierdzenia wiadomoœci do drugiego klienta
				std::cout << "retransmiting: " << receivedComunicate.messageId << std::endl;
				this->retransmit(receivedComunicate, !sender);
				continue;
			}
			else if(receivedComunicate.messageId < 16384)//gdy klient wyœle wiadomoœæ potwierdzamy jej odebranie przez serwer
			{
				std::cout << "sending ack: " << receivedComunicate.messageId << std::endl;
				Comunicate ackComunicate{ 7,3,receivedComunicate.messageId,clients[sender]->sessionId,0,std::vector<UINT8>() };
				sf::Packet ackPacket;
				ackPacket << ackComunicate;
				this->udpSocket.send(ackPacket, receivedIP, receivedPort);
			}

			//interpretacja otrzymanych pakietów
			if (receivedComunicate.operation == 1)//join
			{
				if (receivedComunicate.answer == 0)
				{
					uint32_t sessionId = rand();//losuje nowe id sesji dla nowego klienta
					std::cout << "set new session ID to " << sessionId << std::endl;
					auto newCLient = clients[0];
					if (clients[0] == nullptr)
					{//ustawiam nowego klienta na miejscu 0 w tablicy klientów
						clients[0] = std::make_shared<Client>(Client{ "noname",receivedIP,receivedPort,sessionId,0,0 });
						newCLient = clients[0];
					}
					else
					{
						if (clients[1] == nullptr)
						{//ustawiam nowego klienta na miejscu 1 w tablicy klientów jeœli 0 jest zajêce
							clients[1] = std::make_shared<Client>(Client{ "noname",receivedIP,receivedPort,sessionId,0,0 });
							newCLient = clients[1];
						}
						else
						{
							//gdy nie ma miejsca na serwerze
							std::cout << "nie ma miejsca" << std::endl;
							Comunicate answerComunicate = { 1,6,this->messageId,sessionId,0 ,std::vector<UINT8>() };
							sf::Packet answerPacket;
							answerPacket << answerComunicate;
							this->udpSocket.send(answerPacket,receivedIP,receivedPort);

							continue;
						}
					}
					Comunicate answerComunicate = { 1,7,this->messageId,sessionId,0 ,std::vector<UINT8>() };//tworzê komunikat z potwierdzeniem do³¹czenia do serwera

					sf::Packet answerPacket;
					answerPacket << answerComunicate;

					this->udpSocket.send(answerPacket,receivedIP,receivedPort);//wysy³am komunikat
					this->messageId++;
					//klient dodant
				}
				if (receivedComunicate.answer == 3)//polecenie roz³¹czenia od serwera
				{
					//wys³anie potwierdzenia
					Comunicate answerComunicate{ 1,4,messageId,0,0,std::vector<UINT8>()};
					this->sendTo(answerComunicate, sender);
					clients[sender] = nullptr;//wyzerowanie klienta po stronie serwera

					//poinformowanie innych klientów o od³¹czeniu klienta
					answerComunicate = Comunicate{ 3,3,0,0,0,std::vector<UINT8>() };
					this->sendTo(answerComunicate,!sender);
				}
				if (receivedComunicate.answer == 5)//klient potwierdza do³¹czenie do serwera
				{
					//wys³anie informacji o liczbie klientów
					Comunicate answerComunicate{ 3,0,messageId,0,0,std::vector<UINT8>() };
					if (countClients() == 2)answerComunicate = Comunicate{ 3,1,messageId,0,0,std::vector<UINT8>() };

					if (clients[sender] != nullptr)this->sendTo(answerComunicate, sender);
					if (clients[!sender] != nullptr)this->sendTo(answerComunicate, !sender);
				}
			}

			if (receivedComunicate.operation == 2)//invite
			{
				if (receivedComunicate.answer == 0)//zaproszenie klienta
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
				if (receivedComunicate.answer == 1)//akceptacja zaproszenia klienta przez innego klienta
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
				if (receivedComunicate.answer == 2)//odmowa zaproszenia
				{
					if (clients[sender]->invited == 1)//odmowa
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

			if (receivedComunicate.operation == 7)//wiadomoœæ tekstowa
			{
				if (receivedComunicate.answer == 0)//zwyk³a wiadomoœæ
				{
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
{//przesy³aniekomunikatu do pakietu
	packet.clear();
	std::string msg = "";//najpierw tworzymy string
	msg += std::bitset< 3 >(comunicate.operation).to_string();//dodajemy do stringa kolejne pola zamieniaj¹c je na liczby binarne w postaci ci¹gu znaków
	msg += std::bitset< 3 >(comunicate.answer).to_string();//argument <3> oznacza ¿e zamieniamy comunicate.answer na 3-bitow¹ liczbê zapisan¹ jako ci¹g znaków
	msg += std::bitset< 32 >(comunicate.datasize).to_string();//dodajemy pole rozmiaru danych

	for (auto& letter : comunicate.data)//dodajemy wszystkie litery z komunikatu
	{
		msg += std::bitset< 8 >(int(letter)).to_string();
	}

	msg += std::bitset< 32 >(comunicate.sessionId).to_string();//dodajemy id sesji
	msg += std::bitset< 32 >(comunicate.messageId).to_string();//dodajemy id wiadomoœci

	while (msg.length() % 8 != 0)//uzupe³niamy zerami do d³ugoœci wielokrotnoœci liczby 8
	{
		msg += '0';
	}

	UINT8 pom = 0;
	while (msg.length() > 0)
	{
		std::string pom2 = msg.substr(0, 8);//wczytujemy 8 bitów ze stringa do zmiennej pomocniczej
		pom = std::stoi(pom2, 0, 2);//teraz zamieniamy 8 bitów ze zmiennej pomocniczej na liczbê reprezentowan¹ przez te 8 bitów
		msg.erase(0, 8);//wycinamy ze stringa 8 bitów
		packet << pom;//wrzucamy 8 bitów do pakietu. Czynnoœæ powtarzamy a¿ string bêdzie pusty
	}
	return packet;
}

void operator>>(sf::Packet& packet, Server::Comunicate& comunicate)
{
	std::string msg = "";
	UINT8 pom;

	while (!packet.endOfPacket())
	{
		packet >> pom;//wczytujemy 8 bitów pakietu do zmiennej 8-bitowej
		std::string s = std::bitset< 8 >(pom).to_string();//zamieniamy zmienn¹ 8 bitow¹ na ci¹g znaków binarnych i dodajemy do stringa
		msg += s;
	}

	std::string pom2 = msg.substr(0, 3);//wyci¹gamy 3 bity ze stringa
	msg.erase(0, 3);
	if (pom2.length() == 0)return;
	comunicate.operation = std::stoi(pom2, 0, 2);//zamieniamy wyci¹gniête 3 bity na zmienn¹ i zapisujemy je do komunikatu
												 //czynoœæ powtarzamy dla wszystkich pól

	pom2 = msg.substr(0, 3);
	msg.erase(0, 3);
	if (pom2.length() == 0)return;
	comunicate.answer = std::stoi(pom2, 0, 2);

	pom2 = msg.substr(0, 32);
	msg.erase(0, 32);
	if (pom2.length() == 0)return;
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
	if (pom2.length() != 0)comunicate.messageId = std::stoi(pom2, 0, 2);

}

void Server::print(Comunicate com)
{//procedura wyœwietlaj¹ca zawartoœæ komunikatu
	std::cout << "opr: " << (unsigned int)com.operation << " ans: " << (unsigned int)com.answer << "  mesID: " << (int)com.messageId << " SID: " << (unsigned int)com.sessionId << " datasize: " << (unsigned int)com.datasize << std::endl;
	for (const auto &letter : com.data)
	{
		std::cout << letter << ' ';
	}
	std::cout << std::endl;
}

std::vector<UINT8> Server::toUINTtab(std::string string)
{//zamiania tablicy string, na wektor <UINT8>
	std::vector<UINT8> vector;
	for (const auto& letter : string)
	{
		vector.push_back((UINT8)letter);
	}
	return vector;
}