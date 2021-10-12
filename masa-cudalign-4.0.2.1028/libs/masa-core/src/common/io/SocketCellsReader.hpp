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

#ifndef SOCKETCELLSREADER_HPP_
#define SOCKETCELLSREADER_HPP_

#include "CellsReader.hpp"
#include <string>
using namespace std;

class SocketCellsReader : public CellsReader {
public:
	SocketCellsReader(string hostname, int port);
	virtual ~SocketCellsReader();
	virtual void close();

	virtual int getType();
	virtual int read(cell_t* buf, int len);

private:
    string hostname;
    int port;
    int socketfd;

    void init();
    int resolveDNS(const char* hostname, char* ip);
};

#endif /* SOCKETCELLSREADER_HPP_ */
