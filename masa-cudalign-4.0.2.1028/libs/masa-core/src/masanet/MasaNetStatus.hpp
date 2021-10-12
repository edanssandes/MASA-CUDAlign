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

class MasaNetStatus;

#ifndef MASANETSTATUS_HPP_
#define MASANETSTATUS_HPP_

#include "../libmasa/libmasaTypes.hpp"
#include <pthread.h>
#include <string>
using namespace std;

/* LibMasa Server State */
#define SERVER_SHUTDOWN		(0)
#define SERVER_LISTENING	(1)

/* Processing State */
#define STATE_IDLE			(0)
#define STATE_CREATING_RING	(1)
#define STATE_READY			(2)
#define STATE_STARTING		(3)
#define STATE_PROCESSING	(4)
#define STATE_FINISHING		(5)

class MasaNetStatus {
public:
	MasaNetStatus();
	virtual ~MasaNetStatus();

	void copyFrom(MasaNetStatus* status);

	const score_t& getBestScore();
	void setBestScore(const score_t& bestScore);

	int getLastProcessedRow() const;
	void setLastProcessedRow(int lastProcessedRow);


	int getProcessingState() const;
	void setProcessingState(int processingState);

	int getServerState() const;
	void setServerState(int serverState);

	string toString();

private:
	int serverState;
	int processingState;

	int lastProcessedRow;
	score_t bestScore;
};

#endif /* MASANETSTATUS_HPP_ */
