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

#include "ReversedCellsReader.hpp"

#include <stdio.h>

ReversedCellsReader::ReversedCellsReader(SeekableCellsReader* reader) {
	this->reader = reader;
	this->position = position;
}

ReversedCellsReader::~ReversedCellsReader() {
	close();
}

void ReversedCellsReader::close() {
	if (reader == NULL) {
		reader->close();
		reader = NULL;
	}
}

int ReversedCellsReader::getType() {
	return INIT_WITH_CUSTOM_DATA;
}

int ReversedCellsReader::read(cell_t* buf, int len) {
	if (len > position) {
		len = position;
	}
	position -= len;
	reader->seek(position);
	reader->read(buf, len);

	// Reverse buffer order
	for (int i=0; i<len/2; i++) {
		cell_t aux = buf[i];
		buf[i] = buf[len-1-i];
		buf[len-1-i] = aux;
	}

	return len;
}


void ReversedCellsReader::seek(int position) {
	this->position = position;
}

int ReversedCellsReader::getOffset() {
	return position;
}
