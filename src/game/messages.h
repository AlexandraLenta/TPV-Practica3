// This file is part of the course TPV2@UCM - Samir Genaim

#pragma once
#include <SDL_stdinc.h>
#include "SDLNetUtils.h"

enum MsgId : Uint8 {
	_CONN_REQUEST,
	_CONN_REQUEST_ACCEPTED,
	_CONN_REQUEST_REJECTED,
	_CLIENT_CONNECTED,
	_CLIENT_DISCONNECTED,
	_SERVER_SHUTDOWN,
	_PLAYER_STATE,
	_PLAYER_INFO,
	_SHOOT,
	_DEAD,
	_RESTART,
	_RESTART_TRIGGER
};

struct Msg {
	Uint8 type;

	_IMPL_SERIALIZATION_(type)
};

struct MsgWithClientId : Msg {
	Uint8 clientId;

	_IMPL_SERIALIZATION_(*static_cast<Msg*>(this), clientId)
};

struct MsgWithMasterId : MsgWithClientId {
	Uint8 masterId;

	_IMPL_SERIALIZATION_(*static_cast<MsgWithClientId*>(this), masterId)
};

struct PlayerStateMsg : MsgWithClientId {

	float x;
	float y;
	float rot;

	float oldX;
	float oldY;
	float oldRot;

	_IMPL_SERIALIZATION_(*static_cast<MsgWithClientId*>(this), x, y, rot)

};

struct PlayerInfoMsg : MsgWithClientId {

	float x;
	float y;
	float rot;
	Uint8 state;

	_IMPL_SERIALIZATION_(*static_cast<MsgWithClientId*>(this), x, y, rot, state)

};

struct ShootMsg : MsgWithClientId {

	float x;
	float y;
	float vx;
	float vy;
	int w;
	int h;
	float rot;
	Uint32 timestamp;
	_IMPL_SERIALIZATION_(*static_cast<MsgWithClientId*>(this), x, y, vx, vy, w, h, rot, timestamp)

};

struct DeadMsg : MsgWithClientId {
	Uint32 timestamp;
	Uint8 shooter;

	_IMPL_SERIALIZATION_(*static_cast<MsgWithClientId*>(this), timestamp, shooter)

};

