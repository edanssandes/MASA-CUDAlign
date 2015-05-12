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

#include "InitialCellsReader.hpp"
#include "../../libmasa/IManager.hpp"
#include <stdio.h>

#define DEBUG (0)

InitialCellsReader::InitialCellsReader(int startOffset) {
	this->type = INIT_WITH_ZEROES;
	this->startOffset = startOffset;
	initialize();
}

InitialCellsReader::InitialCellsReader(const int gapOpen, const int gapExt, int startOffset) {
	this->gapOpen = gapOpen;
	this->gapExt = gapExt;
	if (gapOpen == 0 && gapExt == 0) {
		this->type = INIT_WITH_ZEROES;
	} else if (gapOpen == 0) {
		this->type = INIT_WITH_GAPS_OPENED;
	} else {
		this->type = INIT_WITH_GAPS;
	}
	this->startOffset = startOffset;
	initialize();
}

InitialCellsReader::~InitialCellsReader() {
	// Does nothing
}

void InitialCellsReader::close() {
	// Does nothing
}

void InitialCellsReader::seek(int position) {
	this->position = startOffset + position;
}

int InitialCellsReader::getOffset() {
	return position - startOffset;
}

InitialCellsReader* InitialCellsReader::clone(int offset) {
	if (type == INIT_WITH_GAPS || type == INIT_WITH_GAPS_OPENED) {
		return new InitialCellsReader(gapOpen, gapExt, this->startOffset + offset);
	} else {
		return new InitialCellsReader(this->startOffset + offset);
	}
}

int InitialCellsReader::getStartOffset() {
	return startOffset;
}

void InitialCellsReader::initialize() {
	this->position = startOffset;
}

int InitialCellsReader::getType() {
	return this->type;
}

int InitialCellsReader::read(cell_t* buffer, int len) {
	if (DEBUG) fprintf(stdout, "InitialCellsReader::read(%p, %d) - pos: %d\n", buffer, len, position);
	if (buffer != NULL) {
		if (type == INIT_WITH_GAPS || type == INIT_WITH_GAPS_OPENED) {
			// Pre-defined initialization functions
			int k = 0;
			if (position == 0) {
				buffer[0].h = 0;
				buffer[0].f = -INF;
				k++;
			}
			for (; k < len; k++) {
				buffer[k].h = -gapExt*(position+k) - gapOpen;
				buffer[k].e = -INF;
			}
		} else if (type == INIT_WITH_ZEROES) {
			for (int k = 0; k < len; k++) {
				buffer[k].h = 0;
				buffer[k].e = -INF;
			}
		}
	}
	position += len;
	return len;
}

