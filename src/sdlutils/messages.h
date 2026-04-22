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
	_SERVER_SHUTDOWN
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

