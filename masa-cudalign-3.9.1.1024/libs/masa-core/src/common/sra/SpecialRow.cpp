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

#include "SpecialRow.hpp"

#include <stdlib.h>

/*
 * @see description on header file
 */
SpecialRow::SpecialRow() {
	this->offset = 0;
}

/*
 * @see description on header file
 */
SpecialRow::~SpecialRow() {

}

/*
 * @see description on header file
 */
void SpecialRow::open(bool readOnly, int length) {
	this->readOnly = readOnly;
	initialize(readOnly, length);
}

int SpecialRow::getType() {
	return INIT_WITH_CUSTOM_DATA;
}

/*
 * @see description on header file
 */
void SpecialRow::setId(int id) {
	this->id = id;
}

/*
 * @see description on header file
 */
int SpecialRow::getId() {
	return id;
}

/*
 * @see description on header file
 */
int SpecialRow::sortById(SpecialRow* a, SpecialRow* b) {
	if (a == NULL) {
		return true;
	} else if (b == NULL) {
		return false;
	}
    return a->id < b->id;
}

/*
 * @see description on header file
 */
void SpecialRow::seek(int offset) {
	if (!readOnly) {
    	fprintf(stderr, "Cannot change offset of a read-only special row.\n");
    	exit(1);
	}
	this->offset = offset;
}

/*
 * @see description on header file
 */
int SpecialRow::getOffset() {
	return offset;
}

/*
 * @see description on header file
 */
int SpecialRow::write(const cell_t* buf, int len) {
	int ret = write(buf, offset, len);
	if (ret != len) {
    	fprintf(stderr, "Could not write bytes to special row: %d != %d\n", ret, len);
    	perror("SpecialRow::write");
    	exit(1);
	}

	offset += len;
	/*if (DEBUG) {
		fprintf(stderr, "Debug[%s] %d. Tot: %d\n", filename.substr(filename.size()-8,8).c_str(), len, offset);
	}*/


	return len;
}

/*
 * @see description on header file
 */
int SpecialRow::read(cell_t* buf, int len) {
	if (offset == 0) {
		fprintf(stderr, "Error: Special Row overflow: %d (%08X).\n", len, this->id);
		exit(1);
	}
	if (len > offset) {
		len = offset;
	}
	offset -= len;
	if (buf != NULL) {
		int ret = read(buf, offset, len);
		if (ret != len) {
			fprintf(stderr, "Error: End of special row (%d).\n", len-ret);
			exit(1);
		}

		// Reverse buffer order
		for (int i=0; i<len/2; i++) {
			cell_t aux = buf[i];
			buf[i] = buf[len-1-i];
			buf[len-1-i] = aux;
		}
	}

	return len;
}



