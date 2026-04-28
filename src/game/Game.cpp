// This file is part of the course TPV2@UCM - Samir Genaim

#include "Game.h"

#include "../sdlutils/InputHandler.h"
#include "../sdlutils/SDLUtils.h"
#include "LittleWolf.h"
#include "Networking.h"

Game::Game() :
	_little_wolf(nullptr) //
{
}

Game::~Game() {
	delete _little_wolf;

	// close SDLNet
	SDLNetUtils::close_SDLNet();

	// release InputHandler if the instance was created correctly.
	if (InputHandler::HasInstance())
		InputHandler::Release();

	// release SLDUtil if the instance was created correctly.
	if (SDLUtils::HasInstance())
		SDLUtils::Release();

}

bool Game::init(const char* map) {
	_map = map;

	_little_wolf = new LittleWolf();

	// load a map
	_little_wolf->load(_map);

	// initialize the SDL singleton
	if (!SDLUtils::Init("[Little Wolf: " + std::string(map) + "]",
		_little_wolf->get_xres(),
		_little_wolf->get_yres(),
		"resources/config/littlewolf.resources.json")) {

		std::cerr << "Something went wrong while initializing SDLUtils"
			<< std::endl;

		return false;
	}

	// initialize the InputHandler singleton
	if (!InputHandler::Init()) {
		std::cerr << "Something went wrong while initializing SDLHandler"
			<< std::endl;
		return false;

	}	
	
	// initialize SDLNet
	SDLNetUtils::init_SDLNet();

	return true;

}

bool Game::init_game(const char* host, Uint16 port) {
	std::cout << "Initializing game...\n";

	_net = new Networking();

	// establish connection to server first
	if (!_net->init(host, port))
		return false;

	std::cout << "Game initialized!\n";

	_little_wolf->init(sdlutils().window(), sdlutils().renderer());

	// add player
	_little_wolf->addPlayer(_net->get_client_id());

	return true;

}

void Game::start() {

	// a boolean to exit the loop
	bool exit = false;

	auto& ihdlr = ih();

	auto& vt = sdlutils().virtualTimer();
	vt.resetTime();

	while (!exit) {
		Uint32 startTime = vt.regCurrTime();

		// refresh the input handler
		ihdlr.refresh();

		if (ihdlr.keyDownEvent()) {

			// ESC exists the game
			if (ihdlr.isKeyDown(SDL_SCANCODE_ESCAPE)) {
				exit = true;
				continue;
			}
			
			if (ihdlr.isKeyDown(SDL_SCANCODE_R)) {
				_net->send_restart();
			}

		}

		_little_wolf->update();
		_net->update();

		check_collisions();

		// the clear is not necessary since the texture we copy to the window occupies the whole screen
		// sdlutils().clearRenderer();

		_little_wolf->render();
		

		sdlutils().presentRenderer();

		Uint32 frameTime = vt.currRealTime() - startTime;

		if (frameTime < 10)
			SDL_Delay(10 - frameTime);
	}
	_net->disconnect();

}

void Game::check_collisions() {
	if (!_net->is_master())
		return;

	// check if the players are in valid positions
	//for (auto& p : _little_wolf->getPlayers()) {

	//}
}

