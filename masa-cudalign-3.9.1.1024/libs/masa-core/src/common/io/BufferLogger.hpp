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

#ifndef BUFFERLOGGER_HPP_
#define BUFFERLOGGER_HPP_

#include <stdio.h>
#include <string>
using namespace std;

#include "Buffer2.hpp"

class BufferLogger {
public:
	BufferLogger(string file);
	virtual ~BufferLogger();

	void logHeader(int bufferMax);
	void logBuffer(buffer2_statistics_t &curr_stats);
private:
	//Buffer* buffer;
	FILE* file;

	/*float t0;
	int buffer_0;
	int cells_0;
	float wait_0;*/
	buffer2_statistics_t prev_stats;

};

#endif /* BUFFERLOGGER_HPP_ */
