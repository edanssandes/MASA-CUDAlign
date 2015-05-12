/*******************************************************************************
 *
 * Copyright (c) 2010-2015   Edans Sandes
 *
 * This file is part of MASA-Core.
 * 
 * MASA-Core is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * MASA-Core is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MASA-Core.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

class MasaNet;
struct peer_t; // TODO remove

#ifndef MASANET_HPP_
#define MASANET_HPP_

#include <pthread.h>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <sstream>
using namespace std;

#include "MasaNetStatus.hpp"
#include "Peer.hpp"
#include "PeerList.hpp"


/* Node Types */
#define TYPE_CLI				(0)
#define TYPE_PROCESSING_NODE	(1)
#define TYPE_GATEWAY			(2)
#define TYPE_UNKNOWN			(-1)


#define CMD_CONNECTED_PEERS		(0)
#define CMD_DISCOVERED_PEERS	(1)
#define CMD_DATA_PEERS			(2)

typedef void (MasaNet::*cmd_handler_f)(Command* cmd, Peer* socket);


class MasaNet : public MasaNetCallbacks {
public:
	MasaNet(int nodeType, string description);
	virtual ~MasaNet();

	void startServer(int port);
	Peer* connectToPeer(string address, int connection_type);
	void createRing();

	bool onConnect(Peer* peer);
	bool onConnectData(Peer* peer);
	void onDisconnect(Peer* peer);

	void announce(vector<Peer*> announcedPeers, Peer* excludeSocket);
	void disanounce(Peer* peer, Peer* excludeSocket);
	MasaNetStatus* getPeerStatus();
	const set<string>& testRing(string peer);

	const PeerList& getConnectedPeers() const;
	const PeerList& getDiscoveredPeers() const;
	Peer* getLeftPeer() const;
	Peer* getRightPeer() const;

	const vector<Peer*>& getRemotePeers(string peer, int type);

private:
	int nodeType;
	string nodeDescription;

	int serverSocket;
	pthread_t listeningThread;
	bool serverActive;
	pthread_mutex_t mutex;

	MasaNetStatus status;

	string myId;
	int serverPort;

	PeerList peers;
	PeerList discoveredPeers;

	string leftPeerId;
	Peer* leftPeerData;
	Peer* leftPeer;
	string rightPeerId;
	Peer* rightPeerData;
	Peer* rightPeer;


	static map<int, cmd_handler_f> cmdHandlers;


	static void registerCommand(cmd_creator_f creator, cmd_handler_f handler);

	void broadcastCommand(Command* command, Peer* excludeSocket = NULL);

	static void* staticListeningThread(void *arg);
	static void* staticPeerHandler(void *arg);

	static int hostname_to_ip(const char *hostname , char *ip);

	void cmd_discover(Command* _cmd, Peer* socket);
	void cmd_undiscover(Command* _cmd, Peer* socket);
	void cmd_status_request(Command* _cmd, Peer* socket);
	void cmd_peer_request(Command* _cmd, Peer* socket);
	void cmd_create_ring(Command* _cmd, Peer* socket);
	void cmd_test_ring(Command* _cmd, Peer* socket);
	Peer* getPeer(const string& peer);
	Peer* solveSimultaneousConnection(Peer* newPeer, Peer* oldPeer);
};

#endif /* MASANET_HPP_ */
