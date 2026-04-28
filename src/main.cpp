// This file is part of the course TPV2@UCM - Samir Genaim

#include <iostream>
#include <fstream>

#include "game/Game.h"
#include "game/SDLNetUtils.h"
#include "game/TCPServer.h"

void server(Uint16 port) {

	// initialize SDLNet
	SDLNetUtils::init_SDLNet();

	TCPServer server(10);

	if (server.connect(port)) {
		server.listen();
	}

	// close SDLNet
	SDLNetUtils::close_SDLNet();
}

void client(const char* host, Uint16 port) {

	try {
		if (Game::Init("resources/maps/little_wolf/map_1.json")) {
			Game& g = *Game::Instance();
			if (g.init_game(host, port)) {
				g.start();
			}
			Game::Release();
		}
	}
	catch (const std::string& e) { // catch exceptions thrown as strings
		std::cerr << e << std::endl;
	}
	catch (const char* e) { // catch exceptions thrown as char*
		std::cerr << e << std::endl;
	}
	catch (const std::exception& e) { // catch exceptions thrown as a sub-type of std::exception
		std::cerr << e.what();
	}
	catch (...) {
		std::cerr << "Caught an exception of unknown type ...";
	}

}

int main(int argc, char** argv) {

	if (argc == 3 && strcmp(argv[1], "server") == 0) {
		server(static_cast<Uint16>(atoi(argv[2]))); // start in server mode
	}
	else if (argc == 4 && strcmp(argv[1], "client") == 0) {
		client(argv[2], static_cast<Uint16>(atoi(argv[3]))); // start in client mode
	}
	else {
		std::cout << "Usage: " << std::endl;
		std::cout << "  " << argv[0] << " client host port " << std::endl;
		std::cout << "  " << argv[0] << " server port " << std::endl;
		std::cout << std::endl;
		std::cout << "Example:" << std::endl;
		std::cout << "  " << argv[0] << " server 2000" << std::endl;
		std::cout << "  " << argv[0] << " client localhost 2000" << std::endl;
	}

	return 0;
}

