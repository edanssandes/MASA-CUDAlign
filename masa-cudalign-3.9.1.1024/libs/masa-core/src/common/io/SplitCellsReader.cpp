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

#include "SplitCellsReader.hpp"

SplitCellsReader::SplitCellsReader(SeekableCellsReader* reader, int pos0, bool closeAfterUse) {
	this->reader = reader;
	this->pos0 = pos0;
	this->closeAfterUse = closeAfterUse;
}

SplitCellsReader::~SplitCellsReader() {
	close();
}

void SplitCellsReader::seek(int offset) {
	reader->seek(pos0 + offset);
}

int SplitCellsReader::getOffset() {
	return reader->getOffset() - pos0;
}

void SplitCellsReader::close() {
	if (closeAfterUse) {
		reader->close();
	}
}

int SplitCellsReader::getType() {
	return reader->getType();
}

int SplitCellsReader::read(cell_t* buf, int len) {
	reader->read(buf, len);
}
