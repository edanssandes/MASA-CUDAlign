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

class Command;

#ifndef COMMAND_HPP_
#define COMMAND_HPP_

#include "../Peer.hpp"

#define REQUEST_COMMAND		0x1000
#define RESPONSE_COMMAND	0x2000

#define COMMAND_JOIN			(1)
#define COMMAND_LEAVE			(2)
#define COMMAND_NOTIFY_SCORE	(3)
#define COMMAND_DISCOVER		(4)
#define COMMAND_STATUS_REQUEST	(5 | REQUEST_COMMAND)
#define COMMAND_STATUS_RESPONSE	(5 | RESPONSE_COMMAND)
#define COMMAND_PEER_REQUEST	(6 | REQUEST_COMMAND)
#define COMMAND_PEER_RESPONSE	(6 | RESPONSE_COMMAND)
#define COMMAND_CREATE_RING		(7)
#define COMMAND_UNDISCOVER		(8)
#define COMMAND_TEST_RING		(9)

class Command {
public:
	Command();
	virtual ~Command();

	virtual int getId() = 0;

	virtual void send(Peer* socket) = 0;
	virtual void receive(Peer* socket) = 0;

	int getSerial() const;
	void setSerial(int serial);

private:
	int serial;
};

#endif /* COMMAND_HPP_ */
