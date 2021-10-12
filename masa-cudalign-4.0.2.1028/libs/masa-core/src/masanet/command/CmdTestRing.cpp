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

#include "CmdTestRing.hpp"

CmdTestRing::CmdTestRing() {

}

CmdTestRing::CmdTestRing(string originatorId) {
	this->originatorId = originatorId;
}

CmdTestRing::~CmdTestRing() {
}

Command* CmdTestRing::creator() {
	return new CmdTestRing();
}

int CmdTestRing::getId() {
	return COMMAND_TEST_RING;
}

void CmdTestRing::send(Peer* socket) {
	socket->send_vls8(originatorId);
	socket->send_int16(ids.size());
	for (std::set<string>::iterator it=ids.begin(); it!=ids.end(); ++it) {
		printf("-----> %s\n", (*it).c_str());
	    socket->send_vls8(*it);
	}
}

void CmdTestRing::receive(Peer* socket) {
	originatorId = socket->recv_vls8();
	int len = socket->recv_int16();
	for (int i=0; i<len; i++) {
		string remoteId = socket->recv_vls8();
		ids.insert(remoteId);
		printf("<----- %s\n", remoteId.c_str());
	}
}

const set<string>& CmdTestRing::getIds() const {
	return ids;
}

void CmdTestRing::addId(string id) {
	ids.insert(id);
}

bool CmdTestRing::containsId(string id) {
	return ids.find(id) != ids.end();
}

const string& CmdTestRing::getOriginatorId() const {
	return originatorId;
}

void CmdTestRing::setOriginatorId(const string& originatorId) {
	this->originatorId = originatorId;
}
