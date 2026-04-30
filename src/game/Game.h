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

	void string_to_chars(const std::string& str, char c_str[11]) {
		auto i = 0u;
		for (; i < str.size() && i < 10; i++) c_str[i] = str[i];
		c_str[i] = 0;
	}
	void chars_to_string(std::string& str, char c_str[11]) {
		c_str[10] = 0;
		str = std::string(c_str);
	}

private:
	Game();
	virtual ~Game();
	bool init(const char* map);

	Networking* _net;
	LittleWolf* _little_wolf;
	
	const char* _map;

	const int MAX_NAME_LENGTH = 10;
};

