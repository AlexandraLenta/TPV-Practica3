// This file is part of the course TPV2@UCM - Samir Genaim

#include "TCPServer.h"

#include <cassert>
#include <csignal>

#include "messages.h"

Uint16 TCPServer::shutDownTriggetPort = 0;
bool TCPServer::done = true;

TCPServer::TCPServer(Uint8 maxClients) :
	maxClients(maxClients), //
	serv(nullptr), //
	usedConn(0) {
	conn = new void* [maxClients + 1] {};
	clients = new NET_StreamSocket * [maxClients] {};
}

TCPServer::~TCPServer() {
	shutdown();
	delete[] conn;
	delete[] clients;
}

// this just connects to the server to wake it up.
void TCPServer::shutdownTrigger() {

	if (done)
		return;

	std::cout << std::endl << "Shutting down  ..." << std::endl;

	done = true;

	if (shutDownTriggetPort != 0) {

		// Resolve the localhost to an IP address
		NET_Address* addr = NET_ResolveHostname("localhost");
		if (NET_WaitUntilResolved(addr, 1000) != NET_SUCCESS) {
			SDLNetUtils::print_SDLNet_error_and_exit(1);
		}

		// Connect to the server, just to wake it up if it is sleeping
		NET_StreamSocket* conn = NET_CreateClient(addr, shutDownTriggetPort);
		if (NET_WaitUntilConnected(conn, 1000) != NET_SUCCESS) {
			SDLNetUtils::print_SDLNet_error_and_exit(1);
		}

		// UnReference the address so it can be freed
		NET_UnrefAddress(addr);

		// Destroy the socket
		NET_DestroyStreamSocket(conn);
	}
}

/* Signal Handler for SIGINT */
void TCPServer::handlerSIGINT(int) {
	TCPServer::shutdownTrigger();
}

bool TCPServer::connect(const Uint16 port) {

	TCPServer::shutDownTriggetPort = port;
	signal(SIGINT, TCPServer::handlerSIGINT);

	serv = NET_CreateServer(nullptr, port);
	if (!serv) {
#ifndef NDEBUG
		SDLNetUtils::print_SDLNet_error();
#endif
		return false;
	}

	conn[0] = serv;
	++usedConn;

	done = false;

	std::cout << "The server is up and kicking ..." << std::endl;

	return true;
}

void TCPServer::listen() {

	while (!done) {
		// Sleep for a long while, or until someone connects to the server
		if (NET_WaitUntilInputAvailable(conn, usedConn, SDL_MAX_SINT32) > 0) {

			// in case of shutdown wake-up
			if (done)
				continue;

			// We don't know exactly in which socket we had activity, so we check
			// them all.

			// If we have activity in 'serv', we accept the connection
			NET_StreamSocket* client = nullptr;
			if (NET_AcceptClient(serv, &client) && client != nullptr) {

				if (usedConn - 1 < maxClients) {
					Uint8 j = 0;
					while (j < maxClients && clients[j] != nullptr)
						j++;

					assert(j < maxClients);

					// we must have found one, because (usedConn - 1 < maxClients)

					conn[usedConn] = clients[j] = client;
					std::cout << "New client assigned id " << (int)j
						<< std::endl;
					++usedConn;

					// Send acceptance message
					MsgWithMasterId m;
					m.type = _CONN_REQUEST_ACCEPTED;
					m.clientId = j;
					m.masterId = whoIsTheMaster();
					SDLNetUtils::serialized_send(m, client);

					// Tell all, except the new client, that there is a new client
					m.type = _CLIENT_CONNECTED; // the rest of the message remain as before
					for (auto i = 1u; i < usedConn - 1; i++) {
						if (client != static_cast<NET_StreamSocket*>(conn[i]))
							SDLNetUtils::serialized_send(m,
								static_cast<NET_StreamSocket*>(conn[i]));
					}
				}
				else {
					// Reject connection request
					Msg m;
					m.type = _CONN_REQUEST_REJECTED;
					SDLNetUtils::serialized_send(m, client);
				}
			}

			// Check activity in the clients sockets
			for (auto i = 1u; i < usedConn; i++) {

				// Read a message
				SDLNetUtils::buff_t buf = SDLNetUtils::receive(
					static_cast<NET_StreamSocket*>(conn[i]));

				if (!buf.error) { // if no error, we forward it to all (including the sender)
					if (buf.size > 0) {
						for (auto j = 1u; j < usedConn; j++) {
							SDLNetUtils::send(
								static_cast<NET_StreamSocket*>(conn[j]),
								buf.data, buf.size);
						}
					}
				}
				else {
					// the client has disconnected, or an error occurred, so we delete
					// the corresponding socket form the clients array

					// first we find the client id
					Uint8 j = 0;
					while (j < maxClients
						&& clients[j]
						!= static_cast<NET_StreamSocket*>(conn[i]))
						j++;

					assert(j < maxClients);

					std::cout << "Client " << (int)j << " disconnected!"
						<< std::endl;

					// We must have found it, so we remove it
					clients[j] = nullptr;
					conn[i] = conn[usedConn - 1];
					conn[usedConn - 1] = nullptr;
					--usedConn;

					// Tell all clients, that client 'j' has disconnected
					MsgWithMasterId m;
					m.type = _CLIENT_DISCONNECTED;
					m.clientId = j;
					m.masterId = whoIsTheMaster();
					for (auto j = 1u; j < usedConn; j++) {
						SDLNetUtils::serialized_send(m,
							static_cast<NET_StreamSocket*>(conn[j]));
					}
				}
			}
		}
	}

	shutdown();

}

void TCPServer::shutdown() {

	if (serv == nullptr)
		return;

	Msg m;

	m.type = _SERVER_SHUTDOWN;
	// Dispose all client sockets
	for (auto i = 1u; i < usedConn; i++) {
		SDLNetUtils::serialized_send(m,
			static_cast<NET_StreamSocket*>(conn[i]));
		NET_DestroyStreamSocket(static_cast<NET_StreamSocket*>(conn[i]));
	}

	// Close the server's master socket
	NET_DestroyServer(serv);

	serv = nullptr;
}

int TCPServer::whoIsTheMaster() {
	// the master is the one with the minimum id (the first non-null in the clients array)
	for (auto i = 0u; i < maxClients; i++) {
		if (clients[i] != nullptr)
			return i;
	}

	return -1;
}
