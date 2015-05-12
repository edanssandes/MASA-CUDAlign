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

#include "DummyCellsReader.hpp"

#include <string.h>

DummyCellsReader::DummyCellsReader(int size) {
	if (size <= 0) {
		this->size = 0x7FFFFFFF;
	} else {
		this->size = size;
	}
	this->position = 0;
}

DummyCellsReader::~DummyCellsReader() {
	// Does Nothing
}

void DummyCellsReader::close() {
	// Does Nothing
}

int DummyCellsReader::getType() {
	return INIT_WITH_CUSTOM_DATA;
}

int DummyCellsReader::read(cell_t* buf, int len) {
	if (len > size-position) {
		len = size-position;
	}
	memset(buf, 0, sizeof(cell_t)*len);
	this->position += len;
	return len;
}

void DummyCellsReader::seek(int position) {
	this->position = position;
}

int DummyCellsReader::getOffset() {
	return position;
}
