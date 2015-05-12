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

#include "MasaNet.hpp"
#include "command/CmdJoin.hpp"
#include "command/CmdDiscover.hpp"
#include "command/CmdUndiscover.hpp"
#include "command/CmdNotifyScore.hpp"
#include "command/CmdStatusRequest.hpp"
#include "command/CmdStatusResponse.hpp"
#include "command/CmdPeerRequest.hpp"
#include "command/CmdPeerResponse.hpp"
#include "command/CmdCreateRing.hpp"
#include "command/CmdTestRing.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>

#include <string>
#include <algorithm>
using namespace std;


map<int, cmd_handler_f> MasaNet::cmdHandlers;

#define DEFAULT_TCP_PORT	(12777)

MasaNet::MasaNet(int nodeType, string nodeDescription)
{
	this->nodeType = nodeType;
	this->nodeDescription = nodeDescription;
	this->leftPeer = NULL;
	this->leftPeerData = NULL;
	this->rightPeer = NULL;
	this->rightPeerData = NULL;
    timeval event;
    gettimeofday(&event, NULL);
    long long nsec = event.tv_usec%1000000;
	srand(time(NULL)+nsec);
	for (int i=0; i<12; i++) {
		int ic = (rand()%52);
		char c = (ic<26 ? ('A'+ic) : ('a'+ic-26));
		myId += c;
	}
	fprintf(stderr, "MasaNet Name: %s\n", myId.c_str());

	registerCommand(CmdJoin::creator, NULL);
	registerCommand(CmdDiscover::creator, &MasaNet::cmd_discover);
	registerCommand(CmdUndiscover::creator, &MasaNet::cmd_undiscover);
	registerCommand(CmdNotifyScore::creator, NULL);
	registerCommand(CmdStatusRequest::creator, &MasaNet::cmd_status_request);
	registerCommand(CmdStatusResponse::creator, NULL);
	registerCommand(CmdPeerRequest::creator, &MasaNet::cmd_peer_request);
	registerCommand(CmdPeerResponse::creator, NULL);
	registerCommand(CmdCreateRing::creator, &MasaNet::cmd_create_ring);
	registerCommand(CmdTestRing::creator, &MasaNet::cmd_test_ring);

    pthread_mutex_init(&mutex, NULL);

}

MasaNet::~MasaNet() {
    pthread_mutex_destroy(&mutex);
}

void MasaNet::registerCommand(cmd_creator_f creator, cmd_handler_f handler) {
	Command* cmd = creator();
	int id = cmd->getId();
	delete cmd;

	cmdHandlers[id] = handler;
	Peer::registerCommandCreator(id, creator);
}



void MasaNet::startServer(int port) {
    int rc;
    struct sockaddr_in echoServAddr; /* Local address */

    if (port == 0) {
    	serverPort = DEFAULT_TCP_PORT;
    } else {
    	serverPort = port;
    }


    /* Create socket for incoming connections */
    if ((serverSocket = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "ERROR creating server socket; return code from socket() is %d\n", serverSocket);
        exit(-1);
    }

    int optval = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));



    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(serverPort);      /* Local port */

    /* Bind to the local address */
    if ((rc = bind(serverSocket, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr))) < 0) {
        fprintf(stderr, "ERROR; return code from bind() is %d\n", rc);
        exit(-1);
    }

    /* Mark the socket so it will listen for incoming connections */
    if ((rc=listen(serverSocket, 1)) < 0) {
        fprintf(stderr, "ERROR; return code from listen() is %d\n", rc);
        exit(-1);
    }

    serverActive = true;
	fprintf(stderr, "Listening on port %d\n", serverPort);
    rc = pthread_create(&listeningThread, NULL, staticListeningThread, (void *)this);
    if (rc){
        printf("ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
    }

}

Peer* MasaNet::connectToPeer(string address, int connection_type) {
    struct sockaddr_in echoServAddr; /* Echo server address */
    int max_retries = 10;
    int retries = 0;
    int ok = 0;
    int rc;
    int sock;                        /* Socket descriptor */

	char host[256];
	int port = 0;
	if (sscanf(address.c_str(), "%[^:]:%d", host, &port) < 2) {
		//printf("Connecting to host %s\n", host);
	} else {
		//printf("Connecting to host %s:%d\n", host, port);
	}

    if (port == 0) {
    	port = DEFAULT_TCP_PORT;
    }

    char ip[128];
    hostname_to_ip(host, ip);

    /* Construct the server address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));     /* Zero out structure */
    echoServAddr.sin_family      = AF_INET;             /* Internet address family */
    echoServAddr.sin_addr.s_addr = inet_addr(ip);   /* Server IP address */
    echoServAddr.sin_port        = htons(port); /* Server port */

    /* Establish the connection to the echo server */
	fprintf(stderr, "Connecting to server %s (%s) on port %d\n", host, ip, port);

    /* Create a reliable, stream socket using TCP */
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "ERROR creating socket: %s\n", strerror(errno));
		sleep(1);
	}

    while ((retries < max_retries) && !ok) {
		if ((rc=connect(sock, (struct sockaddr *) &echoServAddr, sizeof(sockaddr_in))) < 0) {
			retries++;
			fprintf(stderr, "ERROR connecting to Server [Retry %d/%d]. %s\n",
					retries, max_retries, strerror(errno));
			sleep(1);
		} else {
			ok = 1;
		}
	}
	if (!ok) {
		fprintf(stderr, "ERROR connecting to Server.\n");
		return NULL;
	}
    fprintf(stderr, "Connected to Server %s\n", inet_ntoa(echoServAddr.sin_addr));

	Peer* peersocket = new Peer(sock, myId, true, connection_type);

	pthread_t handshakeThread;
	void** args = new void*[2];
	args[0] = (void*)this;
	args[1] = (void*)peersocket;
    rc = pthread_create(&handshakeThread, NULL, staticPeerHandler, (void *)args);
    if (rc){
        printf("ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
    }
    return peersocket;
}


void* MasaNet::staticListeningThread(void* arg) {
	MasaNet* this_obj = (MasaNet*)arg;
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned int clntLen;            /* Length of client address data structure */
    int clntSock;                    /* Socket descriptor for client */

    /* Set the size of the in-out parameter */
    clntLen = sizeof(echoClntAddr);

    while (this_obj->serverActive) {

		/* Wait for a client to connect */
		fprintf(stderr, "Waiting clients...\n");
		if ((clntSock = accept(this_obj->serverSocket, (struct sockaddr *) &echoClntAddr, &clntLen)) < 0){
			fprintf(stderr, "ERROR; return code from accept() is %d\n", clntSock);
			exit(-1);
		}

		/* clntSock is connected to a client! */

		fprintf(stderr, "Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));

		Peer* peersocket = new Peer(clntSock, this_obj->myId, false);

		pthread_t handshakeThread;
		void** args = new void*[2];
		args[0] = (void*)this_obj;
		args[1] = (void*)peersocket;
		int rc = pthread_create(&handshakeThread, NULL, staticPeerHandler, (void*)args);
	    if (rc){
	        printf("ERROR; return code from pthread_create() is %d\n", rc);
	        exit(-1);
	    }

    }

    close(this_obj->serverSocket); // TODO necessary?

    return NULL;

}

void* MasaNet::staticPeerHandler(void* arg) {
	void** args = (void**)arg;
	MasaNet* this_obj = (MasaNet*)(args[0]);
	Peer* socket = (Peer*)(args[1]);

	socket->setLocalType(this_obj->nodeType);

	/* Publish the public address instead of the spurious client port */
	if (this_obj->serverPort != 0) {
		char desc[256];
		sprintf(desc, ":%d", this_obj->serverPort);
		socket->setLocalAddress(string(desc));
	}

	socket->setCallback(this_obj);
	if (!socket->handshake()) {
		/*for (int i=0; i<10 + (this_obj->serverPort%4)*2; i++) {
			fprintf(stderr, "Will disconnect %p\n", socket);
			sleep(1);
		}*/
		delete socket;
	}

	if (socket->getConnectionType() == CONNECTION_TYPE_DATA) {
		return NULL;
	}

	int locked = false;
	try {
		while (socket->isConnected()) {
			fprintf(stderr, "Waiting Command... %p\n", socket);
			Command* cmd = socket->recvCommand();
			if (cmd == NULL) {
				fprintf(stderr, "Interrupting Command listener\n");
				break;
			}

			cmd_handler_f handler = cmdHandlers[cmd->getId()];
			fprintf(stderr, "ReceivedCommand: %d\n", cmd->getId());
			if (handler != NULL) {
				fprintf(stderr, "Acquiring Lock\n");
				pthread_mutex_lock(&this_obj->mutex);
				locked = true;
				fprintf(stderr, "Acquiring Lock: OK\n");
				(this_obj->*handler)(cmd, socket);
				pthread_mutex_unlock(&this_obj->mutex);
				locked = false;
				fprintf(stderr, "Unlocked\n");
			}

		}
	} catch(IOException &e) {
		fprintf(stderr, "IOException: %s (%s) %p\n", e.what(), socket->getRemoteId().c_str(), socket);
		if (locked) {
			pthread_mutex_unlock(&this_obj->mutex);
		}
	}


	return NULL;
}

void MasaNet::broadcastCommand(Command* command, Peer* excludeSocket) {
	vector<Peer*> allPeers = peers.getProcessingPeers();
	for (vector<Peer*>::iterator it = allPeers.begin() ; it != allPeers.end(); ++it) {
		if ((*it) == excludeSocket) continue;
		if ((*it)->getRemoteType() == TYPE_PROCESSING_NODE) { // TODO retirar?
			(*it)->sendCommand(command);
		}
	}
}

/*
    Get ip from domain name
 */
int MasaNet::hostname_to_ip(const char *hostname , char *ip) {
	// Source: http://www.binarytides.com/hostname-to-ip-address-c-sockets-linux/
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in *h;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // use AF_INET6 to force IPv6
    hints.ai_socktype = SOCK_STREAM;

    if ( (rv = getaddrinfo( hostname , "http" , &hints , &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next)
    {
        h = (struct sockaddr_in *) p->ai_addr;
        strcpy(ip , inet_ntoa( h->sin_addr ) );
    }

    freeaddrinfo(servinfo); // all done with this structure
    return 0;
}

Peer* MasaNet::solveSimultaneousConnection(Peer* newPeer, Peer* oldPeer) {
	if ( (newPeer->getRemoteId() < myId) ^ (newPeer->isInitiator()) ) {
		newPeer->finalize();
		return oldPeer;
	} else {
		oldPeer->finalize();
		return newPeer;
	}
}

bool MasaNet::onConnect(Peer* peer) {
	fprintf(stderr, "**** ON CONNECT\n");

	pthread_mutex_lock(&this->mutex);
	if (peers.get(peer->getRemoteId()) != NULL) {
		Peer* preferrablePeer = solveSimultaneousConnection(peer, peers.get(peer->getRemoteId()));
		if (preferrablePeer == peer) {
			printf("Peer already connected %s %p. Replacing\n", peer->getRemoteId().c_str(), peer);
			peers.erase(peer->getRemoteId());
		} else {
			pthread_mutex_unlock(&this->mutex);
			printf("Peer already connected %s %p. Aborting\n", peer->getRemoteId().c_str(), peer);
			return false;
		}
	}

	peers.add(peer);
	printf("node connected: %s\n", peer->getRemoteId().c_str());

	if (peer->getRemoteId() == this->leftPeerId) {
		this->leftPeer = peer;
		peer->ringType = RING_LEFT;
	} else if (peer->getRemoteId() == this->rightPeerId) {
		this->rightPeer = peer;
		peer->ringType = RING_RIGHT;
	} else {
		peer->ringType = RING_NONE;
	}
	printf("node connected: %s.. %s|%s %d\n", peer->getRemoteId().c_str(), this->leftPeerId.c_str(), this->rightPeerId.c_str(), peer->ringType);


	if (peer->getRemoteType() == TYPE_PROCESSING_NODE) {

		if (this->discoveredPeers.get(peer->getRemoteId()) == NULL) {
			this->discoveredPeers.add(peer);
			printf("node discovered: %s\n", peer->getRemoteId().c_str());

			CmdDiscover cmd;
			cmd.addPeers(this->discoveredPeers.getProcessingPeers());
			peer->sendCommand(&cmd);

			vector<Peer*> announcedNode;
			announcedNode.push_back(peer);
			this->announce(announcedNode, peer); // broadcast
		} else {
			this->discoveredPeers.add(peer);
			printf("node updated: %s\n", peer->getRemoteId().c_str());
		}
	}
	pthread_mutex_unlock(&this->mutex);

	return true;
}

bool MasaNet::onConnectData(Peer* peer) {
	fprintf(stderr, "**** ON CONNECT DATA **********\n");
	bool successful = true;

	pthread_mutex_lock(&this->mutex);

	if (peer->getRemoteId() == this->leftPeerId) {
		if (leftPeerData == NULL) {
			this->leftPeerData = peer;
		} else {
			this->leftPeerData = solveSimultaneousConnection(peer, this->leftPeerData);
			if (this->leftPeerData == peer) {
				printf("Data peer already connected %s %p. Replacing\n", peer->getRemoteId().c_str(), peer);
			} else {
				printf("Data Peer already connected %s %p. Aborting\n", peer->getRemoteId().c_str(), peer);
				successful = false;
			}
		}
	} else if (peer->getRemoteId() == this->rightPeerId) {
		if (rightPeerData == NULL) {
			this->rightPeerData = peer;
		} else {
			this->rightPeerData = solveSimultaneousConnection(peer, this->rightPeerData);
			if (this->rightPeerData == peer) {
				printf("Data peer already connected %s %p. Replacing\n", peer->getRemoteId().c_str(), peer);
			} else {
				printf("Data Peer already connected %s %p. Aborting\n", peer->getRemoteId().c_str(), peer);
				successful = false;
			}
		}
	}
	pthread_mutex_unlock(&this->mutex);

	return successful;
}


void MasaNet::onDisconnect(Peer* peer) {
	fprintf(stderr, "**** ON DISCONNECT !!!!\n");


	bool sendDisanounce = false;
	pthread_mutex_lock(&this->mutex);
	if (this->peers.get(peer->getRemoteId()) == NULL &&
			this->discoveredPeers.get(peer->getRemoteId()) != NULL) {
		this->peers.erase(peer->getRemoteId());
		sendDisanounce = true;
		this->discoveredPeers.erase(peer->getRemoteId());
	}
	pthread_mutex_unlock(&this->mutex);

	//if (sendDisanounce) {
		//this->disanounce(peer, NULL);
	//}
}


const PeerList& MasaNet::getConnectedPeers() const {
	return peers;
}

const PeerList& MasaNet::getDiscoveredPeers() const {
	return discoveredPeers;
}






void MasaNet::announce(vector<Peer*> announcedPeers, Peer* excludeSocket) {
	CmdDiscover cmd;
	if (announcedPeers.size() == 0) {
		return;
	}
	cmd.addPeers(announcedPeers);
	broadcastCommand(&cmd, excludeSocket);
}

void MasaNet::disanounce(Peer* peer, Peer* excludeSocket) {
	CmdUndiscover cmd;
	if (peer == NULL) {
		return;
	}
	cmd.addPeer(peer);
	fprintf(stderr, "Disanouncing: %s\n", peer->getRemoteId().c_str());
	broadcastCommand(&cmd, excludeSocket);
}



MasaNetStatus* MasaNet::getPeerStatus() {
	vector<Peer*> allPeers = peers.getProcessingPeers();
	for (vector<Peer*>::iterator it = allPeers.begin() ; it != allPeers.end(); ++it) {
		CmdStatusRequest request;
		CmdStatusResponse* response = (CmdStatusResponse*)(*it)->sendCommand(&request);
		if (response != NULL) {
			status = response->getStatus();
			// FIXME Memory leak - response
			return &status;
		}
	}
	return NULL;
}

Peer* MasaNet::getPeer(const string& peer) {
	Peer* socket;
	if (peer.length() == 0) {
		socket = peers.getProcessingPeers().front();
	} else {
		socket = peers.get(peer);
	}
	return socket;
}

const vector<Peer*>& MasaNet::getRemotePeers(string peer, int type) {
	vector<Peer*> remotePeers;
	CmdPeerRequest request;
	request.setType(type);
	Peer* socket = getPeer(peer);
	CmdPeerResponse* response = (CmdPeerResponse*)socket->sendCommand(&request);
	return response->getPeers();
}

void MasaNet::createRing() {
	CmdCreateRing cmd;
	broadcastCommand(&cmd);
}


const set<string>& MasaNet::testRing(string peer) {
	CmdTestRing cmd(myId);

	Peer* socket = getPeer(peer);
	socket->addHook(cmd.getId());
	socket->sendCommand(&cmd);
	CmdTestRing* async = (CmdTestRing*)socket->waitHook();
	set<string> ret = async->getIds();

	return ret;
}



void MasaNet::cmd_discover(Command* _cmd, Peer* socket) {
	fprintf(stderr, "Cmd Discover %p\n", _cmd);
	CmdDiscover* cmd = (CmdDiscover*)_cmd;
	vector<Peer*> receivedPeers = cmd->getPeers();

	vector<Peer*> newPeers;
	for (vector<Peer*>::iterator it = receivedPeers.begin() ; it != receivedPeers.end(); ++it) {
		if (discoveredPeers.get((*it)->getRemoteId()) == NULL) {
			newPeers.push_back(*it);
			discoveredPeers.add(*it);
			printf("New Node: %s\n", (*it)->getRemoteId().c_str());
		}
	}

	announce(newPeers, socket);

	/*vector<peer_t> diff;
	for (map<string, peer_t>::iterator it = getConnectedPeers().begin() ; it != getConnectedPeers().end(); ++it) {
		sendCommand(it->second.socket, cmd);
	}
	std::set_difference(peers.begin(), peers.end(), this->discoveredPeers.begin(), this->discoveredPeers.end(),
				std::inserter(result, result.end()));

	fprintf(stderr, "Command Discover: %d\n", peers.size());
	int len0 = this->discoveredPeers.size();
	this->discoveredPeers.insert(peers.begin(), peers.end());
	int len1 = this->discoveredPeers.size();
	for (set<string>::iterator it=peers.begin(); it!=peers.end(); ++it) {
		fprintf(stderr, "-> %s\n", it->c_str());
	}
	if (true || len0 != len1) {
		for (map<string, peer_t>::iterator it = getConnectedPeers().begin() ; it != getConnectedPeers().end(); ++it) {
			sendCommand(it->second.socket, cmd);
		}
	}*/
}

void MasaNet::cmd_undiscover(Command* _cmd, Peer* socket) {
	fprintf(stderr, "Cmd Undiscover %p\n", _cmd);
	/*CmdUndiscover* cmd = (CmdUndiscover*)_cmd;
	vector<Peer*> receivedPeers = cmd->getPeers();
	Peer* peer = receivedPeers.front();

	if (discoveredPeers.get(peer->getRemoteId()) != NULL) {
		if (peers.get(peer->getRemoteId()) == NULL) {
			printf("Deleting Node: %s\n", peer->getRemoteId().c_str());
			discoveredPeers.erase(peer->getRemoteId());
			//discoveredPeers.copy(peers.getProcessingPeers());
			disanounce(peer, socket);
			//announce(peers.getProcessingPeers(), NULL);
		} else {
			announce(receivedPeers, NULL);
		}
	}*/

}

void MasaNet::cmd_status_request(Command* _cmd, Peer* socket) {
	fprintf(stderr, "Cmd Status Request %p\n", _cmd);
	CmdStatusResponse response(&status);
	response.setSerial(_cmd->getSerial());
	socket->sendCommand(&response);
}

void MasaNet::cmd_peer_request(Command* _cmd, Peer* socket) {
	CmdPeerRequest* request = (CmdPeerRequest*)_cmd;;
	fprintf(stderr, "Cmd Peer Request %p (%d)\n", _cmd, request->getType());
	CmdPeerResponse response;
	if (request->getType() == CMD_CONNECTED_PEERS) {
		response.addPeers(peers.getProcessingPeers());
	} else if (request->getType() == CMD_DISCOVERED_PEERS) {
		response.addPeers(discoveredPeers.getProcessingPeers());
	} else if (request->getType() == CMD_DATA_PEERS) {
		response.addPeer(leftPeerData);
		response.addPeer(rightPeerData);
	}
	response.setSerial(_cmd->getSerial());
	socket->sendCommand(&response);
}

Peer* MasaNet::getLeftPeer() const {
	return leftPeer;
}

Peer* MasaNet::getRightPeer() const {
	return rightPeer;
}

void MasaNet::cmd_create_ring(Command* _cmd, Peer* socket) {
	fprintf(stderr, "Cmd Create Ring %p\n", _cmd);

	if (status.getProcessingState() != STATE_IDLE) {
		printf("Server is not idle: %d.\n", status.getProcessingState());
		return;
	}
	status.setProcessingState(STATE_CREATING_RING);
	printf("Creating Ring.\n");

	createRing();

	Peer* prev = discoveredPeers.getPrev(myId);
	leftPeerId = prev->getRemoteId();
	Peer* next = discoveredPeers.getNext(myId);
	rightPeerId = next->getRemoteId();

	printf("I must connect to nodes: %s and %s\n", leftPeerId.c_str(), rightPeerId.c_str());

	if (peers.get(rightPeerId) == NULL) {
		connectToPeer(next->getRemoteAddress(), CONNECTION_TYPE_CTRL);
	} else {
		rightPeer = next;
		rightPeer->ringType = RING_RIGHT;
	}

	if (peers.get(leftPeerId) == NULL) {
		connectToPeer(prev->getRemoteAddress(), CONNECTION_TYPE_CTRL);
	} else {
		leftPeer = prev;
		leftPeer->ringType = RING_LEFT;
	}

	if (rightPeerData == NULL) {
		connectToPeer(next->getRemoteAddress(), CONNECTION_TYPE_DATA);
	}
	if (leftPeerData == NULL) {
		connectToPeer(next->getRemoteAddress(), CONNECTION_TYPE_DATA);
	}

}


void MasaNet::cmd_test_ring(Command* _cmd, Peer* socket) {
	fprintf(stderr, "Cmd Test Ring %p\n", _cmd);
	CmdTestRing* cmd = (CmdTestRing*)_cmd;

	if (status.getProcessingState() == STATE_IDLE) {
		printf("Ring is not created.\n");
		return;
	}
	if (cmd->containsId(myId)) {
		// Loop finished
		if (cmd->getOriginatorId() != myId) {
			cmd->addId(myId);
			peers.get(cmd->getOriginatorId())->sendCommand(cmd);
		} else {
			string firstId = *(cmd->getIds().begin());
			if (firstId != myId) {
				fprintf(stderr, "Ring is not fully connected: %d peer(s)\n", cmd->getIds().size());
			} else {
				fprintf(stderr, "Ring is fully connected with %d peer(s)\n", cmd->getIds().size());
			}
		}
	} else {
		cmd->addId(myId);

		Peer* next = getRightPeer();
		next->sendCommand(cmd);
	}
}
