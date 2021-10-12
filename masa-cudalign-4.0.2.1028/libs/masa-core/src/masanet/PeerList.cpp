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

#include "PeerList.hpp"

PeerList::PeerList() {
	// TODO Auto-generated constructor stub

}

PeerList::~PeerList() {
	// TODO Auto-generated destructor stub
}

void PeerList::add(Peer* peer) {
	peerMap[peer->getRemoteId()] = peer;
}

Peer* PeerList::get(const string& id) const {
	map<string, Peer*>::const_iterator i = peerMap.find(id);
	if (i == peerMap.end()) {
		return NULL;
	} else {
		return i->second;
	}
}

void PeerList::erase(const string& id) {
	peerMap.erase(id);
}

void PeerList::copy(vector<Peer*> copy) {
	peerMap.clear();
	for (vector<Peer*>::iterator it = copy.begin() ; it != copy.end(); ++it) {
		peerMap[(*it)->getRemoteId()] = *it;
	}
}

Peer* PeerList::getPrev(const string& id) const {
	map<string, Peer*>::const_iterator i = peerMap.find(id);
	if (i == peerMap.end()) {
		return NULL;
	} else {
		i--;
		if (i == peerMap.end()) {
			return (peerMap.rbegin())->second;
		} else {
			return i->second;
		}
	}
}

Peer* PeerList::getNext(const string& id) const {
	map<string, Peer*>::const_iterator i = peerMap.find(id);
	if (i == peerMap.end()) {
		return NULL;
	} else {
		i++;
		if (i == peerMap.end()) {
			return (peerMap.begin())->second;
		} else {
			return i->second;
		}
	}
}

vector<Peer*> PeerList::getAllPeers() const {
	vector<Peer*> peers;
	for (map<string, Peer*>::const_iterator it = peerMap.begin() ; it != peerMap.end(); ++it) {
		peers.push_back(it->second);
	}
	return peers;
}

vector<Peer*> PeerList::getProcessingPeers() const {
	vector<Peer*> peers;
	for (map<string, Peer*>::const_iterator it = peerMap.begin() ; it != peerMap.end(); ++it) {
		if (it->second->getRemoteType() == TYPE_PROCESSING_NODE) {
			peers.push_back(it->second);
		}
	}
	return peers;
}
