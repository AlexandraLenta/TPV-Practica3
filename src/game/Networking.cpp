// This file is part of the course TPV2@UCM - Samir Genaim

#include "Networking.h"

#include <cassert>

#include "../sdlutils/SDLUtils.h"
#include "Game.h"
#include "LittleWolf.h"

Networking::Networking() :
	sock(nullptr), _client_Id(0), _master_Id(0) {
}

Networking::~Networking() {
	if (sock != nullptr)
		NET_DestroyStreamSocket(sock);
}

bool Networking::init(const char* host, Uint16 port) {
	bool success = false;

	Msg m0;
	MsgWithMasterId m1;

	// The address of the server we want to connect to
	NET_Address* addr = NET_ResolveHostname(host);
	if (NET_WaitUntilResolved(addr, 1000) != NET_SUCCESS) {
		NET_UnrefAddress(addr);
		return success;
	}

	// Connect to the server
	sock = NET_CreateClient(addr, port);
	if (NET_WaitUntilConnected(sock, 1000) != NET_SUCCESS) {
		NET_DestroyStreamSocket(sock);
		NET_UnrefAddress(addr);
		sock = nullptr;
		return success;
	}

	// we do not need the address anymore
	NET_UnrefAddress(addr);

	void* conn[1];
	conn[0] = sock;

	// - wait for the server to send acceptance message
	if (NET_WaitUntilInputAvailable(conn, 1, 1000) > 0) {
		SDLNetUtils::buff_t buf = SDLNetUtils::receive(sock);
		// forward it
		if (!buf.error) {
			if (buf.size > 0) {
				m0.deserialize(buf.data);

				if (m0.type == _CONN_REQUEST_ACCEPTED) {
					m1.deserialize(buf.data);
					_client_Id = m1.clientId;
					_master_Id = m1.masterId;
					success = true;
					std::cout << "Connected with id " << (int)_client_Id
						<< " (the current master is client "
						<< (int)_master_Id << ")" << std::endl;

				}
				else {
					std::cout << "Connection request has been rejected!"
						<< std::endl;
				}
			}
			else {
				std::cout << "Something went wrong ..." << std::endl;
			}
		}
		else {
			std::cout << "Something went wrong ..." << std::endl;
		}
	}
	else {
		std::cout << "Did not get reply on time ..." << std::endl;
	}

	return success;
}

void Networking::disconnect() {
	if (sock == nullptr)
		return;
	NET_DestroyStreamSocket(sock);
	sock = nullptr;
}

void Networking::update() {
	assert(sock != nullptr);

	Msg m0;
	MsgWithMasterId m1;
	PlayerStateMsg m2;
	ShootMsg m3;
	MsgWithClientId m4;
	PlayerInfoMsg m5;
	DeadMsg m6;
	PlayerDmgInfoMsg m7;
	PlayerScoreInfoMsg m8;

	while (true) {
		SDLNetUtils::buff_t buf = SDLNetUtils::receive(sock);

		// if the server close the connection
		if (buf.error) {
			std::cout
				<< "Something went wrong with the connection to the server!"
				<< std::endl;
			SDLNetUtils::print_SDLNet_error_and_exit(1);
		}

		// we get nothing
		if (buf.size == 0)
			break;

		// handle the message
		m0.deserialize(buf.data);
		switch (m0.type) {
		case _CLIENT_CONNECTED:
			m1.deserialize(buf.data);
			_master_Id = m1.masterId;
			handle_new_client(m1.clientId);
			break;

		case _CLIENT_DISCONNECTED:
			m1.deserialize(buf.data);
			_master_Id = m1.masterId;
			handle_disconnet(m1.clientId);
			break;

		case _PLAYER_STATE:
			m2.deserialize(buf.data);
			handle_player_state(m2);
			break;

		case _PLAYER_INFO:
			m5.deserialize(buf.data);
			handle_player_info(m5);
			break;

		case _SHOOT:
			m3.deserialize(buf.data);
			handle_shoot(m3);
			break;

		case _DEAD:
			m6.deserialize(buf.data);
			handle_dead(m6);
			break;

		case _RESTART:
			handle_restart();
			break;

		case _RESTART_TRIGGER:
			handle_restart_trigger();
			break;

		case _DAMAGE:
			m7.deserialize(buf.data);
			handle_damaged(m7);
			break;
			
		case _SCORE:
			m8.deserialize(buf.data);
			handle_score(m8);
			break;

		default:
			break;
		}
	}
}

void Networking::send_state(const Vector2D& pos, float rot, const Vector2D& oldPos, float oldRot) {
	PlayerStateMsg m;
	m.type = _PLAYER_STATE;
	m.clientId = _client_Id;
	m.x = pos.getX();
	m.y = pos.getY();
	m.rot = rot;
	m.oldX = oldPos.getX();
	m.oldY = oldPos.getY();
	m.oldRot = oldRot;
	SDLNetUtils::serialized_send(m, sock);
}

void Networking::send_my_info(const Vector2D& pos, float rot, int hp, int score,
	Uint8 state) {
	PlayerInfoMsg m;
	m.type = _PLAYER_INFO;
	m.clientId = _client_Id;
	m.x = pos.getX();
	m.y = pos.getY();
	m.rot = rot;
	m.hp = hp;
	m.score = score;
	m.state = state;
	SDLNetUtils::serialized_send(m, sock);
}

void Networking::send_damaged_info(Uint8 id, int hp) {
	PlayerDmgInfoMsg m;
	m.type = _DAMAGE;
	m.clientId = id;
	m.hp = hp;
	SDLNetUtils::serialized_send(m, sock);
}

void Networking::send_score_info(Uint8 id, int score) {
	PlayerScoreInfoMsg m;
	m.type = _DAMAGE;
	m.clientId = id;
	m.score = score;
	SDLNetUtils::serialized_send(m, sock);
}

void Networking::send_dead(Uint8 id, Uint8 shooter, Uint32 timestamp) {
	DeadMsg m;
	m.type = _DEAD;
	m.clientId = id;
	m.shooter = shooter;
	m.timestamp = timestamp;
	SDLNetUtils::serialized_send(m, sock);
}

void Networking::send_shoot(Uint8 id) {
	ShootMsg m;
	m.type = _SHOOT;
	m.clientId = _client_Id; 
	SDLNetUtils::serialized_send(m, sock);
}

void Networking::send_restart() {
	Msg m;
	m.type = _RESTART;
	SDLNetUtils::serialized_send(m, sock);
}

void Networking::send_restart_trigger() {
	Msg m;
	m.type = _RESTART_TRIGGER;
	SDLNetUtils::serialized_send(m, sock);
}

void Networking::handle_new_client(Uint8 id) {
	if (id != _client_Id)
		Game::Instance()->get_wolves().send_my_info();
}

void Networking::handle_disconnet(Uint8 id) {
	Game::Instance()->get_wolves().removePlayer(id);
}

void Networking::handle_player_state(const PlayerStateMsg& m) {
	if (m.clientId != _client_Id) {
		Game::Instance()->get_wolves().update_player_state(m.clientId, m.x,
			m.y, m.rot);
	}

	if (_client_Id == _master_Id) {
		auto* playerList = Game::Instance()->get_wolves().getPlayers();
		int maxPlayers = Game::Instance()->get_wolves().getMaxPlayers();
		for (int i = 0; i < maxPlayers; i++) {

		}
	}
}

void Networking::handle_player_info(const PlayerInfoMsg& m) {
	if (m.clientId != _client_Id) {
		Game::Instance()->get_wolves().update_player_info(m.clientId, m.x,
			m.y, m.rot, m.hp, m.score, m.state);
	}
}

void Networking::handle_shoot(const ShootMsg& m) {
	Game::Instance()->get_wolves().play_shootSFX(m.clientId, LittleWolf::SFX::GUNSHOT); // send gunshot sound to each player

	if (_client_Id == _master_Id) {
		int victimID = Game::Instance()->get_wolves().shoot(m.clientId);

		if (victimID != -1) {
			Game::Instance()->get_wolves().play_shootSFX(m.clientId, LittleWolf::SFX::PAIN); // send damage sound to each player
			if (Game::Instance()->get_wolves().is_dead(victimID)) // if the victim is dead we send the death message
				send_dead(victimID, m.clientId, SDL_GetTicks());
		}
	}
}

void Networking::handle_dead(const DeadMsg& m) {
	Game::Instance()->get_wolves().killPlayer(m.clientId);
	Game::Instance()->get_wolves().play_shootSFX(m.clientId, LittleWolf::SFX::PAIN); // play pain sound for every player

	if (_client_Id == _master_Id) { // the master checks if game should restart
		Game::Instance()->get_wolves().check_restart();
	}
}

void Networking::handle_restart() {
	Game::Instance()->get_wolves().restart();
}

void Networking::handle_restart_trigger() {
	Game::Instance()->get_wolves().triggerRestart();
}

void Networking::handle_damaged(const PlayerDmgInfoMsg& m) {
	Game::Instance()->get_wolves().update_player_hp(m.clientId, m.hp);
}

void Networking::handle_score(const PlayerScoreInfoMsg& m) {
	Game::Instance()->get_wolves().update_player_score(m.clientId, m.score);
}