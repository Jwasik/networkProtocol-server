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

int main()
{
	Server server;
	server.run();
	return 0;
}

