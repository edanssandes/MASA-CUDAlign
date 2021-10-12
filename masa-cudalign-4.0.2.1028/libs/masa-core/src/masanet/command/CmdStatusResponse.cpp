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

#include "CmdStatusResponse.hpp"

CmdStatusResponse::CmdStatusResponse() {
}

CmdStatusResponse::CmdStatusResponse(MasaNetStatus* status) {
	this->status.copyFrom(status);
}

CmdStatusResponse::~CmdStatusResponse() {

}

Command* CmdStatusResponse::creator() {
	return new CmdStatusResponse();
}

int CmdStatusResponse::getId() {
	return COMMAND_STATUS_RESPONSE;
}

void CmdStatusResponse::send(Peer* socket) {
	socket->send_int8(status.getProcessingState());
	socket->send_int8(status.getServerState());

	socket->send_int32(status.getBestScore().i);
	socket->send_int32(status.getBestScore().j);
	socket->send_int32(status.getBestScore().score);
	socket->send_int32(status.getLastProcessedRow());
}

void CmdStatusResponse::receive(Peer* socket) {
	status.setProcessingState(socket->recv_int8());
	status.setServerState(socket->recv_int8());

	score_t best;
	best.i = socket->recv_int32();
	best.j = socket->recv_int32();
	best.score = socket->recv_int32();
	status.setBestScore(best);
	status.setLastProcessedRow(socket->recv_int32());
}

const MasaNetStatus& CmdStatusResponse::getStatus() const {
	return status;
}
