// networkProtocol.cpp : Ten plik zawiera funkcję „main”. W nim rozpoczyna się i kończy wykonywanie programu.
//

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <memory>
#include <vector>
#include <SFML/Network.hpp>
#include "Server.h"

/*class aaa
{
	std::string& toString(std::vector<UINT8>&);
	void sendMsg(sf::UdpSocket&, std::shared_ptr<Client>, std::string, bool);
	void sendMsg(sf::UdpSocket&, Client&, std::string, bool);


	int main()
	{

	
	std::vector<UINT8>& toUINTtab(std::string string)
	{
		std::vector<UINT8> vector;
		for (const auto& letter : string)
		{
			vector.push_back((UINT8)letter);
		}
		return vector;
	}

	std::string& toString(std::vector<UINT8>& vector)
	{
		std::string string;
		for (const auto& element : vector)
		{
			string.push_back((unsigned char)element);
		}
		return string;
	}

	std::string std::to_string(Comunicate& com1)
	{

		std::string text;
		for (auto& it : com1.data)
		{
			std::cout << it;
			text += (char)it;
		}
		std::cout << std::endl << text << std::endl;
		return text;
	}

}
*/
int main()
{
	Server server;
	server.run();
	return 0;
}

