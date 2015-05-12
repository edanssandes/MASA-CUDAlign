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

#include "CmdPeerList.hpp"

CmdPeerList::CmdPeerList() {
}

CmdPeerList::CmdPeerList(vector<Peer*> peers) {
	this->peers = peers;
}

CmdPeerList::~CmdPeerList() {

}

void CmdPeerList::send(Peer* socket) {
	socket->send_int16(peers.size());
	printf("----- %d\n", peers.size());
	for (std::vector<Peer*>::iterator it=peers.begin(); it!=peers.end(); ++it) {
	    socket->send_vls8((*it)->getRemoteId());
	    socket->send_vls8((*it)->getRemoteAddress());
	    socket->send_int8((*it)->getRemoteType());
	    socket->send_int8((*it)->getConnectionType());
	    socket->send_int8((*it)->ringType);
	}
}

void CmdPeerList::receive(Peer* socket) {
	int len = socket->recv_int16();
	for (int i=0; i<len; i++) {
		string remoteId = socket->recv_vls8();
		string remoteAddress = socket->recv_vls8();
		int remoteType = socket->recv_int8();
		int connectionType = socket->recv_int8();
		int ringType = socket->recv_int8();

		Peer* peer = new Peer(remoteId, remoteAddress, remoteType, ringType, connectionType);
		peers.push_back(peer);
		printf("----- %s (%d)\n", peer->getRemoteId().c_str(), peer->getConnectionType());
	}
}

void CmdPeerList::addPeer(Peer* peer) {
	if (peer != NULL) {
		peers.push_back(peer);
	}
}

void CmdPeerList::addPeers(vector<Peer*> peers) {
	for (std::vector<Peer*>::iterator it=peers.begin(); it!=peers.end(); ++it) {
		addPeer(*it);
	}
}

const vector<Peer*>& CmdPeerList::getPeers() {
	return peers;
}
