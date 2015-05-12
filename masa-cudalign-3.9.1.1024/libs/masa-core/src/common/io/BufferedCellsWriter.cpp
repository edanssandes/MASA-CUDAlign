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

#include "BufferedCellsWriter.hpp"

#include <stdio.h>
#include <stdlib.h>

#define DEBUG (0)

BufferedCellsWriter::BufferedCellsWriter(CellsWriter* writer, int bufferLimit) {
    if (writer == NULL){
        printf("BufferedCellsWriter::ERROR; null writer\n");
        exit(-1);
    }
	this->writer = writer;
	if (writer != NULL) {
		initBuffer(bufferLimit);
	}
}

BufferedCellsWriter::~BufferedCellsWriter() {
	close();
}

void BufferedCellsWriter::close() {
	printf("BufferedCellsWriter::close()\n");
	if (writer != NULL) {
		destroyBuffer();
		writer->close();
		writer = NULL;
	}
}

int BufferedCellsWriter::write(const cell_t* buf, int len) {
	return writeBuffer(buf, len);
}

void BufferedCellsWriter::bufferLoop() {

    while (!isBufferDestroyed()) {
    	cell_t cell;
        int len = readBuffer(&cell, 1);
        if (len <= 0) break;
    	len = writer->write(&cell, 1);
        if (len <= 0) break;
    }
	if (DEBUG) printf("BufferedCellsWriter::bufferLoop() - DONE\n");
}
