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

#include "Peer.hpp"

#include "command/Command.hpp"
#include "MasaNet.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
using namespace std;

#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#define MAGIC_STRING	"MASA_NET"
#define MAGIC_ANSWER	"MASA_ACK"

#define MASA_NET_VERSION_MAJOR	0
#define MASA_NET_VERSION_MINOR	1

#define FLAG_NONE			0x00000000

#define SEND_ERROR_MSG	("send: socket error")
#define RECV_ERROR_MSG	("recv: socket error")

map<int, cmd_creator_f> Peer::cmdCreators;
bool Peer::hasStaticEvent = false;
timeval Peer::staticEvent;

Peer::Peer(int socket, const string& localId, bool initiator, int connectionType) {
	this->socket = socket;
	this->connectionType = connectionType;
	this->initiator = initiator;
	this->connected = false;
	this->serial = 0;
	this->localId = localId;
	this->remoteType = TYPE_UNKNOWN;
	this->localType = TYPE_UNKNOWN;
	this->ringType = RING_NONE;
	this->error = false;
	this->handshakeDone = false;

	struct sockaddr_in sin;
	socklen_t len = sizeof(sin);
	if (getsockname(socket, (struct sockaddr *)&sin, &len) == -1) {
	    perror("getsockname");
	}

	char desc[256];
	sprintf(desc, "%s:%d", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
	localAddress = string(desc);

	if (getpeername(socket, (struct sockaddr *)&sin, &len) == -1) {
	    perror("getpeername");
	}
	sprintf(desc, "%s:%d", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
	remoteAddress = string(desc);

	fprintf(stderr, "Peer::Peer(%d, %s, %d): %p [%s]\n", socket, localId.c_str(), initiator, this, desc);

	pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&responseCond, NULL);
    pthread_cond_init(&handshakeCond, NULL);

    /* This code will prevent the SIGPIPE signal from being raised, but you
     * will get a read / write error when trying to use the socket, so you
     * will need to check for that.
     */
    signal(SIGPIPE, SIG_IGN);
}

Peer::Peer(string remoteId, string remoteAddress,
		const int remoteType, const int ringType, int connectionType) {
	this->remoteId = remoteId;
	this->remoteAddress = remoteAddress;
	this->remoteType = remoteType;
	this->ringType = ringType;
	this->localType = TYPE_UNKNOWN;
	this->connected = false;
	this->serial = 0;
	this->initiator = false;
	this->socket = 0;
	this->error = false;
	this->handshakeDone = false;
	this->connectionType = connectionType;

	// TODO criar mutex, conds?
}

Peer::~Peer() {
	this->connected = false;
	pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&responseCond);
    pthread_cond_destroy(&handshakeCond);
}

bool Peer::handshake() {
	if (initiator) {
		connected = handshakeInitiator();
	} else {
		connected = handshakeInitiated();
	}
	pthread_mutex_lock(&mutex);
	handshakeDone = true;
	pthread_cond_broadcast(&handshakeCond);
	pthread_mutex_unlock(&mutex);

	if (connected) {
		if (connectionType == CONNECTION_TYPE_DATA) {
			connected = callback->onConnectData(this);
		} else {
			connected = callback->onConnect(this);
		}
	}

	if (!connected) {
		finalize();
	}
	return connected;
}

void Peer::finalize() {
	if (socket != 0) {
		fprintf(stderr, "Peer::finalize (%s) %d\n", remoteId.c_str(), socket);
		close(socket);
		socket = 0;
	}
}

bool Peer::waitHandshake() {
	printf("Wait 1\n");
	pthread_mutex_lock(&mutex);
	printf("Wait 2\n");
	while (!handshakeDone) {
		printf("Wait 3\n");
		pthread_cond_wait(&handshakeCond, &mutex);
	}
	printf("Wait 4\n");
	pthread_mutex_unlock(&mutex);
	return connected;
}

bool Peer::handshakeInitiator() {
	char magic[32];
	recv_array(magic, strlen(MAGIC_STRING));
	if (memcmp(magic, MAGIC_STRING, strlen(MAGIC_STRING)) != 0) {
        fprintf(stderr, "handshake: wrong magic string: %s/%s %d\n", MAGIC_STRING, magic, socket);
        return 0;
	}

	send_array(MAGIC_ANSWER, strlen(MAGIC_ANSWER));

	int major = recv_int8();
	int minor = recv_int8();

	if (major != MASA_NET_VERSION_MAJOR) {
        fprintf(stderr, "handshake: unsupported version: %d.%d\n", major, minor);
        return 0;
	}

	int flags = FLAG_NONE;
	send_int32(flags);

	send_int8(connectionType);

	send_vls8(localId);
	remoteId = recv_vls8();

	send_int16(localType);
	remoteType = recv_int16();

	send_vls8(localAddress);

	return 1;
}

string Peer::toString() {
    stringstream msg;
	msg << remoteId << "[";
	switch (remoteType) {
		case TYPE_CLI:
			msg << "CL";
			break;
		case TYPE_PROCESSING_NODE:
			msg << "PE";
			break;
		case TYPE_GATEWAY:
			msg << "GW";
			break;
		case TYPE_UNKNOWN:
			msg << "??";
			break;
	}
	msg << "](" << remoteAddress << ")";
	if (ringType == RING_RIGHT) {
		msg << " *R";
	} else if (ringType == RING_LEFT) {
		msg << " *L";
	}
	if (connectionType == CONNECTION_TYPE_CTRL) {
		msg << " CTRL";
	} else if (connectionType == CONNECTION_TYPE_DATA) {
		msg << " DATA";
	} else {
		msg << " ??";
	}
	return msg.str();
}

void Peer::registerCommandCreator(int id, cmd_creator_f creator) {
	cmdCreators[id] = creator;
}

bool Peer::handshakeInitiated() {
	send_array(MAGIC_STRING, strlen(MAGIC_STRING));

	char magic[32];
	recv_array(magic, strlen(MAGIC_ANSWER));
	if (memcmp(magic, MAGIC_ANSWER, strlen(MAGIC_ANSWER)) != 0) {
        fprintf(stderr, "handshake: wrong magic answer: %s/%s %d\n", MAGIC_ANSWER, magic, socket);
        return 0;
	}

	send_int8(MASA_NET_VERSION_MAJOR);
	send_int8(MASA_NET_VERSION_MINOR);

	int flags = recv_int32();

	connectionType = recv_int8();

	remoteId = recv_vls8();
	send_vls8(localId);

	remoteType = recv_int16();
	send_int16(localType);

	remoteAddress = recv_vls8();

	return 1;
}


bool Peer::isConnected() {
	return connected;
}

void Peer::addHook(int id, int serial) {
	pthread_t tid = pthread_self();

	pthread_mutex_lock(&mutex);
	hookThreads[id].insert(tid);
	hookSerial[tid] = serial;
	hookCommand[tid] = NULL;
	pthread_mutex_unlock(&mutex);
}

Command* Peer::waitHook() {
	pthread_t tid = pthread_self();
	Command* ret = NULL;

	pthread_mutex_lock(&mutex);
	while (hookCommand[tid] == NULL) {
		pthread_cond_wait (&responseCond, &mutex);
		// TODO colocar timeout?
		// FIXME tratar caso de desconexão
	}
	ret = hookCommand[tid];
	hookCommand[tid] = NULL;
	hookThreads[ret->getId()].erase(tid);
	pthread_mutex_unlock(&mutex);
	return ret;
}

void Peer::notifyHook(Command* cmd) {
	int id = cmd->getId();
	pthread_mutex_lock(&mutex);
	printf("Hook Received %d. %d\n", id, hookThreads[id].size());
	if (hookThreads[id].size() > 0) {
		bool wakeup = false;
		for (std::set<pthread_t>::iterator it=hookThreads[id].begin(); it!=hookThreads[id].end(); ++it) {
			printf("Hook Received: %d %d\n", id, hookSerial[*it]);
			if (hookSerial[*it] == -1 || hookSerial[*it] == cmd->getSerial()) {
				hookCommand[*it] = cmd;
				hookThreads[id].erase(*it);
				wakeup = true;
			}
		}
		if (wakeup) {
			pthread_cond_broadcast(&responseCond);
		}
	}
	pthread_mutex_unlock(&mutex);
}

int Peer::getNextSerial() {
	return ++serial;
}


int Peer::getLocalType() const {
	return localType;
}

void Peer::setLocalType(int localType) {
	this->localType = localType;
}

int Peer::getRemoteType() const {
	return remoteType;
}

const string& Peer::getLocalAddress() const {
	return localAddress;
}

const string& Peer::getRemoteAddress() const {
	return remoteAddress;
}

const string& Peer::getLocalId() const {
	return localId;
}

const string& Peer::getRemoteId() const {
	return remoteId;
}

void Peer::setLocalAddress(const string& publicAddress) {
	if (publicAddress[0] != ':') {
		this->localAddress = publicAddress;
	} else {
		int port;
		sscanf(publicAddress.c_str(), ":%d", &port);

		struct sockaddr_in sin;
		socklen_t len = sizeof(sin);
		if (getsockname(socket, (struct sockaddr *)&sin, &len) == -1) {
		    perror("getsockname");
		}
		char desc[256];
		sprintf(desc, "%s:%d", inet_ntoa(sin.sin_addr), port);
		this->localAddress = string(desc);
	}
}

Command* Peer::sendCommand(Command* command) {
	fprintf(stderr, "Will Lock: %d \n", command->getId());
	pthread_mutex_lock(&mutex);
	fprintf(stderr, " Locked 1\n");
	int id = command->getId();
	fprintf(stderr, " Locked 2\n");
	send_int16(id);
	fprintf(stderr, " Locked 3\n");
	if (id & REQUEST_COMMAND) {
		int serial = getNextSerial();
		command->setSerial(serial);
		send_int32(serial);
	} else if (id & RESPONSE_COMMAND) {
		send_int32(command->getSerial());
	}
	fprintf(stderr, " Locked 4\n");
	command->send(this);
	fprintf(stderr, "sendCommand(%s->%s): %d.%d \n", localId.c_str(), remoteId.c_str(), id, command->getSerial());



	Command* ret = NULL;
	if (id & REQUEST_COMMAND) {
		pthread_t tid = pthread_self();
		hookThreads[(id ^ REQUEST_COMMAND) | RESPONSE_COMMAND].insert(tid);
		hookSerial[tid] = serial;
		hookCommand[tid] = NULL;
		printf("hook: %d %d\n", (id ^ REQUEST_COMMAND) | RESPONSE_COMMAND, serial);
		while (hookCommand[tid] == NULL) {
			fprintf(stderr, "wait response\n");
			pthread_cond_wait (&responseCond, &mutex);
		}
		ret = hookCommand[tid];
	}

	pthread_mutex_unlock(&mutex);

	return ret;

}


Command* Peer::recvCommand() {
	fprintf(stderr, "recvCommand(%s<-%s): Waiting... %p\n", localId.c_str(), remoteId.c_str(), this);
	int id = recv_int16();
	int serial = 0;
	if ((id & REQUEST_COMMAND) || (id & RESPONSE_COMMAND)) {
		serial = recv_int32();
	}
	cmd_creator_f creator = cmdCreators[id];
	if (creator == NULL) {
		fprintf(stderr, "cmd: unsupported command [%d]\n", id);
		// TODO
	}
	Command* cmd = creator();
	cmd->setSerial(serial);
	cmd->receive(this);
	fprintf(stderr, "recvCommand(%s<-%s): %d.%d \n", localId.c_str(), remoteId.c_str(), id, cmd->getSerial());

	/*if (id & RESPONSE_COMMAND) {
		pthread_mutex_lock(&mutex);
		response = cmd;
		pthread_cond_broadcast (&responseCond);
		while (response != NULL && waitingResponse > 0) {
			pthread_cond_wait (&responseCond, &mutex);
			// TODO colocar timeout?
			// FIXME tratar caso de disconexão
		}
		pthread_mutex_unlock(&mutex);
	}*/

	notifyHook(cmd);

	return cmd;
}



void Peer::handleSendError(int ret) {
    if (ret <= 0) {
    	callback->onDisconnect(this);
        throw IOException(SEND_ERROR_MSG);
    }
}

void Peer::handleRecvError(int ret) {
    if (ret <= 0) {
    	perror("Error reading from server");
    	fprintf(stderr, "Ret: %d (%d)\n", ret, socket);

    	callback->onDisconnect(this);
        throw IOException(RECV_ERROR_MSG);
    }
}


void Peer::send_int32(int value) {
	int msg = htonl(value);
	int ret = send(socket, &msg, sizeof(msg), 0);
	handleSendError(ret);
}

int Peer::recv_int32() {
	int msg;
	int ret = recv(socket, &msg, sizeof(msg), 0);
	handleRecvError(ret);

    return ntohl(msg);
}

void Peer::send_int16(short value) {
	short msg = htons(value);
	int ret = send(socket, &msg, sizeof(msg), 0);
	handleSendError(ret);
}

short Peer::recv_int16() {
	short msg;
	int ret = recv(socket, &msg, sizeof(msg), 0);
	handleRecvError(ret);

    return ntohs(msg);
}

void Peer::send_int8(char value) {
	char msg = value;
	int ret = send(socket, &msg, sizeof(msg), 0);
	handleSendError(ret);
}

char Peer::recv_int8() {
	char msg;
	int ret = recv(socket, &msg, sizeof(msg), 0);
	handleRecvError(ret);

    return msg;
}

void Peer::send_array(const char* value, int len) {
	int ret = send(socket, value, len, 0);
	handleSendError(ret);
}

void Peer::recv_array(char* value, int len) {
	char msg;
	int ret = recv(socket, value, len, 0);
	handleRecvError(ret);

}

void Peer::send_vls8(const char* value) {
	int len = strlen(value);
	if (len > 255) {
		len = 255;
	}
	send_int8(len);
	send_array(value, len);
}

void Peer::recv_vls8(char* value, int max) {
	unsigned char len = (unsigned char)recv_int8();
	//fprintf(stderr, "recv_vls8: %d\n", len);
	if (max != -1 && len > max-1) {
		recv_array(value, max-1);
		value[max-1] = '\0';
		recv_dummy(len-max+1);
	} else {
		recv_array(value, len);
		value[len] = '\0';
	}
}

void Peer::send_vls8(const string& value) {
	send_vls8(value.c_str());
}

string Peer::recv_vls8() {
	char str[256];
	recv_vls8(str, sizeof(str));
	return string(str);
}

void Peer::recv_dummy(int len) {
	char dummy[len];
	recv_array(dummy, len);
}

float Peer::getGlobalTime() {
	if (!hasStaticEvent) {
		hasStaticEvent = true;
		gettimeofday(&staticEvent, NULL);
	}

	timeval event;
	gettimeofday(&event, NULL);

	float diff = getElapsedTime(&event, &staticEvent);
	return diff;
}

void Peer::setCallback(MasaNetCallbacks* callback) {
	this->callback = callback;
}

bool Peer::isInitiator() const {
	return initiator;
}

int Peer::getSocket() const {
	return socket;
}

int Peer::getConnectionType() const {
	return connectionType;
}

float Peer::getElapsedTime(timeval *end_time, timeval *start_time) {
	timeval temp_diff;

	temp_diff.tv_sec = end_time->tv_sec - start_time->tv_sec;
	temp_diff.tv_usec = end_time->tv_usec - start_time->tv_usec;

	if (temp_diff.tv_usec < 0) {
		long long nsec = temp_diff.tv_usec/1000000;
		temp_diff.tv_usec += 1000000 * nsec;
		temp_diff.tv_sec -= nsec;
	}

	return (1000000LL * temp_diff.tv_sec + temp_diff.tv_usec)/1000.0f;

}
