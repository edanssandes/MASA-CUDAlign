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

#ifndef CMDDISCOVER_HPP_
#define CMDDISCOVER_HPP_

#include "Command.hpp"
#include "CmdPeerList.hpp"

#include <string>
#include <set>
using namespace std;

#include "../MasaNet.hpp"

class CmdDiscover : public CmdPeerList {
public:
	CmdDiscover();
	CmdDiscover(vector<Peer*> peers);
	virtual ~CmdDiscover();

	static Command* creator();

	virtual int getId();
};

#endif /* CMDDISCOVER_HPP_ */
