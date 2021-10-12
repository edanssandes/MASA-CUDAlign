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

#include "BufferedStream.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "BufferLogger.hpp"

BufferedStream::BufferedStream() {
	this->buffer = NULL;
}

BufferedStream::~BufferedStream() {
	destroyBuffer();
}

void BufferedStream::destroyBuffer() {
	if (buffer != NULL) {
		buffer->waitEmptyBuffer();
		buffer->destroy();
		pthread_join(threadId, NULL);
		delete buffer;
		buffer = NULL;
	}
}

bool BufferedStream::isBufferDestroyed() {
	return buffer->isDestroyed();
}

int BufferedStream::readBuffer(cell_t* buf, int len) {
	return buffer->readBuffer(buf, len);
}

int BufferedStream::writeBuffer(const cell_t* buf, int len) {
	return buffer->writeBuffer(buf, len);
}

void BufferedStream::initBuffer(int bufferLimit) {
	this->buffer = new Buffer2(bufferLimit);

    int rc = pthread_create(&threadId, NULL, staticThreadFunction, (void *)this);
    if (rc){
        printf("ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
    }
}

void* BufferedStream::staticThreadFunction(void* arg) {
	BufferedStream* _this = (BufferedStream*)arg;
    _this->bufferLoop();
    return NULL;
}

void BufferedStream::setLogFile(string logFile, float interval) {
	buffer->setLogFile(logFile, interval);
}
