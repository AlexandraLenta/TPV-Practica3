// This file is part of the course TPV2@UCM - Samir Genaim

#pragma once


class LittleWolf;

class Game {
public:
	Game();
	virtual ~Game();
	void init(const char* map);
	void start();
private:
	LittleWolf* _little_wolf;

};

