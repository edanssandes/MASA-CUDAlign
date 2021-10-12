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

#ifndef BUFFEREDSTREAM_HPP_
#define BUFFEREDSTREAM_HPP_

#include "Buffer2.hpp"
#include "BufferLogger.hpp"

class BufferedStream {
public:
	BufferedStream();
	virtual ~BufferedStream();

    void setLogFile(string logFile, float interval);

protected:
	void initBuffer(int bufferLimit);
	void destroyBuffer();
	bool isBufferDestroyed();
	int readBuffer(cell_t* buf, int len);
	int writeBuffer(const cell_t* buf, int len);
    virtual void bufferLoop() = 0;

private:
    Buffer2* buffer;
    BufferLogger* logger;
    pthread_t threadId;

    static void* staticThreadFunction(void *arg);
};

#endif /* BUFFEREDSTREAM_HPP_ */
