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

	void send_state(const Vector2D& pos, float rot, const Vector2D& oldPos, float oldRot);
	void send_my_info(const Vector2D& pos, float rot, int hp, int score,
		Uint8 state);
	void send_shoot(Uint8 id);
	void send_damaged_info(Uint8 id, int hp);
	void send_score_info(Uint8 id, int score);

	void send_dead(Uint8 id, Uint8 shooter, Uint32 timestamp);
	void send_restart();
	void send_restart_trigger();
	void send_name_set(Uint8 id, const std::string name);

private:

	void handle_new_client(Uint8 id);
	void handle_disconnet(Uint8 id);
	void handle_player_state(const PlayerStateMsg& m);
	void handle_player_info(const PlayerInfoMsg& m);
	void handle_shoot(const ShootMsg& m);
	void handle_dead(const DeadMsg& m);
	void handle_restart();
	void handle_restart_trigger();
	void handle_damaged(const PlayerDmgInfoMsg& m);
	void handle_score(const PlayerScoreInfoMsg& m);
	void handle_name_set(NameMsg& m);

	NET_StreamSocket* sock;
	Uint8 _client_Id;
	Uint8 _master_Id;
};

