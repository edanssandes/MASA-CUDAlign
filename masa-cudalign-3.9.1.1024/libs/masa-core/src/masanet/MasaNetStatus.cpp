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

#include "MasaNetStatus.hpp"
#include <sstream>

MasaNetStatus::MasaNetStatus() {
    this->serverState = SERVER_SHUTDOWN;
    this->processingState = STATE_IDLE;
    this->lastProcessedRow = -1;
    this->bestScore.score = -INF;
    this->bestScore.i = -1;
    this->bestScore.j = -1;
}

MasaNetStatus::~MasaNetStatus() {
}

void MasaNetStatus::copyFrom(MasaNetStatus* status) {
    this->bestScore = status->bestScore;
    this->lastProcessedRow = status->lastProcessedRow;
    this->processingState = status->processingState;
    this->serverState = status->serverState;
}



const score_t& MasaNetStatus::getBestScore() {
    return bestScore;
}

void MasaNetStatus::setBestScore(const score_t& bestScore) {
	this->bestScore = bestScore;
}

int MasaNetStatus::getLastProcessedRow() const {
	return lastProcessedRow;
}

void MasaNetStatus::setLastProcessedRow(int lastProcessedRow) {
	this->lastProcessedRow = lastProcessedRow;
}

int MasaNetStatus::getProcessingState() const {
	return processingState;
}

void MasaNetStatus::setProcessingState(int processingState) {
	this->processingState = processingState;
}

int MasaNetStatus::getServerState() const {
	return serverState;
}


void MasaNetStatus::setServerState(int serverState) {
	this->serverState = serverState;
}

string MasaNetStatus::toString() {
    stringstream msg;

	msg <<  "\nServer State:\t ";
	switch (getServerState()) {
		case SERVER_SHUTDOWN: msg <<  "Shutdown"; break;
		case SERVER_LISTENING: msg <<  "Listening"; break;
	}

	msg << "\nProc. Status:\t ";
	switch (getProcessingState()) {
		case STATE_IDLE: msg << "Idle"; break;
		case STATE_STARTING: msg << "Starting"; break;
		case STATE_PROCESSING: msg << "Processing"; break;
		case STATE_FINISHING: msg << "Finishing"; break;
	}

	msg << "\nLastRow:\t " << getLastProcessedRow();
	msg << "\nBestScore:\t ";
	int score = getBestScore().score;
	if (score == -INF) {
		msg << "-INF";
	} else {
		msg << score;
	}
	msg << "@(" << getBestScore().i << "," << getBestScore().j << ")";
	return msg.str();
}
