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

class Peer;

#ifndef PEER_HPP_
#define PEER_HPP_

#include <pthread.h>
#include <sys/time.h>

#include <string>
#include <map>
#include <set>
using namespace std;

#include "command/Command.hpp"
#include "../common/exceptions/IOException.hpp"
#include "MasaNetCallbacks.hpp"

/* Node Types */
#define TYPE_CLI				(0)
#define TYPE_PROCESSING_NODE	(1)
#define TYPE_GATEWAY			(2)
#define TYPE_UNKNOWN			(-1)


/* Ring Types */
#define RING_NONE			(0)
#define RING_RIGHT			(+1)	//Local to Remote
#define RING_LEFT			(-1)	//Remote to Local

#define CONNECTION_TYPE_UNKNOWN	(0)
#define CONNECTION_TYPE_CTRL	(1)
#define CONNECTION_TYPE_DATA	(2)

typedef Command* (*cmd_creator_f)();

class Peer {
public:
	int ringType; // TODO Proteger

	Peer(int socket, const string& localId, bool initiator, int connectionType = CONNECTION_TYPE_UNKNOWN);
	Peer(string remoteId, string remoteAddress,
			const int remoteType, const int ringType, const int connectionType);
	virtual ~Peer();

	static void registerCommandCreator(int id, cmd_creator_f creator);
	Command* recvCommand();
	Command* sendCommand(Command* command);

	void addHook(int id, int serial = -1);
	Command* waitHook();

	bool isConnected();
	bool handshake();
	bool waitHandshake();
	void finalize();

	void  send_int32(int value);
	int   recv_int32();
	void  send_int16(short value);
	short recv_int16();
	void  send_int8(char value);
	char  recv_int8();
	void  send_array(const char* value, int len);
	void  recv_array(char* value, int len);
	void  send_vls8(const char* value);
	void  recv_vls8(char* value, int max=-1);
	void  send_vls8(const string& value);
	string recv_vls8();

	void recv_dummy(int len);


	void setLocalType(int localType);
	void setLocalAddress(const string& publicAddress);

	const string& getLocalAddress() const;
	const string& getRemoteAddress() const;
	const string& getLocalId() const;
	const string& getRemoteId() const;
	int getLocalType() const;
	int getRemoteType() const;

	string toString();
	void setCallback(MasaNetCallbacks* callback);
	bool isInitiator() const;
	int getSocket() const;
	int getConnectionType() const;

private:
	MasaNetCallbacks* callback;

	pthread_mutex_t mutex;
	pthread_cond_t responseCond;
	pthread_cond_t handshakeCond;
	//int waitingResponse;

	//Command* response;

	int socket;
	bool initiator;
	bool handshakeDone;
	bool connected;
	bool error;
	int serial;
	float timeout;

	int connectionType;

	static map<int, cmd_creator_f> cmdCreators;

	map<int, set<pthread_t> > hookThreads;
	map<pthread_t, Command*> hookCommand;
	map<pthread_t, int> hookSerial;

	string localId;
	string remoteId;
	string localAddress;
	string remoteAddress;
	int remoteType;
	int localType;

	bool handshakeInitiator();
	bool handshakeInitiated();

	int getNextSerial();

	void notifyHook(Command* cmd);

	void handleSendError(int ret);
	void handleRecvError(int ret);

	static bool hasStaticEvent;
	static timeval staticEvent;
	static float getGlobalTime();
    static float getElapsedTime(timeval *end_time, timeval *start_time);


};


#endif /* PEER_HPP_ */
