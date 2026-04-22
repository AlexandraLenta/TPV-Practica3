// This file is part of the course TPV2@UCM - Samir Genaim

#pragma once

#include <cstddef>
#include "SDLNetUtils.h"

class TCPServer {
public:
	TCPServer(Uint8 maxClients);
	virtual ~TCPServer();
	bool connect(const Uint16 port);
	void listen();

private:

	// these methods/fields are used to capture CTRL-C so we can perform a clean shutdown
	static bool done;
	static Uint16 shutDownTriggetPort;
	static void shutdownTrigger();
	static void handlerSIGINT(int);

	// to shutdown the server;
	void shutdown();

	// a method to calculate the id of the master (the one with the minimum id)
	int whoIsTheMaster();

	// the maximum number of clients, we just allow up to 255 (to
	// send only one byte for the client id).
	Uint8 maxClients;

	// the socket of the server;
	NET_Server* serv;

	// this array hold all open connections **without gaps**. It is used
	// as a socket-set to check for activities. The server will be in the
	// first position, the rest are clients
	void** conn;
	std::size_t usedConn;

	// This array will store the sockets of the clients: client with identifier
	// 'i' will be stored at position 'i'. The array might have gaps.
	NET_StreamSocket** clients;

};

