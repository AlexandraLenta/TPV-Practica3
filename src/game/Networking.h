// This file is part of the course TPV2@UCM - Samir Genaim

#pragma once
#include <SDL_stdinc.h>

#include "../utils/Vector2D.h"
#include "messages.h"

class Networking {
public:
	Networking();
	virtual ~Networking();

	bool init(const char* host, Uint16 port);
	void disconnect();
	void update();

	Uint8 get_client_id() {
		return _client_Id;
	}

	bool is_master() {
		return _client_Id == _master_Id;
	}

	void send_state(const Vector2D& pos, float rot);
	void send_my_info(const Vector2D& pos, float rot,
		Uint8 state);

	void send_shoot(Vector2D p, Vector2D v, int width, int height, float r);
	void send_dead(Uint8 id, Uint8 shooter, Uint32 timestamp);
	void send_restart();

private:

	void handle_new_client(Uint8 id);
	void handle_disconnet(Uint8 id);
	void handle_player_state(const PlayerStateMsg& m);
	void handle_player_info(const PlayerInfoMsg& m);
	void handle_shoot(const ShootMsg& m);
	void handle_dead(const DeadMsg& m);
	void handle_restart();

	NET_StreamSocket* sock;
	Uint8 _client_Id;
	Uint8 _master_Id;
};

