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

#include "BufferedCellsReader.hpp"

#include <stdio.h>
#include <stdlib.h>

#define DEBUG (0)

BufferedCellsReader::BufferedCellsReader(CellsReader* reader, int bufferLimit) {
    if (reader == NULL){
        printf("BufferedCellsReader::ERROR; null reader\n");
        exit(-1);
    }
	this->reader = reader;
	this->offset = 0;

	initBuffer(bufferLimit);
}

BufferedCellsReader::~BufferedCellsReader() {
	close();
}

void BufferedCellsReader::close() {
	if (DEBUG) fprintf(stderr, "BufferedCellsReader::close().\n");
	if (reader != NULL) {
		reader->close();
	}
	destroyBuffer();
	if (reader != NULL) {
		reader = NULL;
	}
	if (DEBUG) fprintf(stderr, "BufferedCellsReader::close() DONE.\n");
}

int BufferedCellsReader::getType() {
	return reader->getType();
}

int BufferedCellsReader::read(cell_t* buf, int len) {
	int ret = readBuffer(buf, len);
	offset += ret;
	return ret;
}

void BufferedCellsReader::seek(int position) {
	fprintf(stderr, "BufferedCellsReader: Cannot seek.\n");
	exit(1);
}

int BufferedCellsReader::getOffset() {
	return offset;
}

void BufferedCellsReader::bufferLoop() {
    while (!isBufferDestroyed()) {
    	cell_t cell;
    	int len = reader->read(&cell, 1);
        if (len <= 0) break;
        len = writeBuffer(&cell, 1);
        if (len <= 0) break;
    }
	if (DEBUG) printf("BufferedCellsReader::bufferLoop() - DONE\n");
}


