// This file is part of the course TPV2@UCM - Samir Genaim

#pragma once

#include "../utils/Singleton.h"
#include <SDL_stdinc.h>

class LittleWolf;
class Networking;

class Game : public Singleton<Game> {
	friend Singleton<Game>;

public:
	bool init_game(const char* host, Uint16 port);
	void start();

	inline Networking& get_networking() {
		return *_net;
	}

	LittleWolf& get_wolves() {
		return *_little_wolf;
	}
private:
	Game();
	virtual ~Game();
	bool init(const char* map);
	void check_collisions();

	Networking* _net;
	LittleWolf* _little_wolf;
	
	const char* _map;
};

