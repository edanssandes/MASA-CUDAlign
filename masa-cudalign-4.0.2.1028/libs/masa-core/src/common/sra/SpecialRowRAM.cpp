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

#include "SpecialRowRAM.hpp"

#include <stdlib.h>

/** Initial row size in number of elements */
#define INITIAL_LENGTH		(1048*1048)

/** Multiplier of the length whenever the row is full */
#define LENGTH_MULTIPLIER 	(1.5) // 50%

static int sum = 0;

/*
 * @see description on header file
 */
SpecialRowRAM::SpecialRowRAM(int id)
{
	length = 0;
	row = NULL;
	setId(id);
}

/*
 * @see description on header file
 */
SpecialRowRAM::~SpecialRowRAM() {
	if (row != NULL) {
		//printf("Closed %p\n", row);
		free(row);
		row = NULL;
	}
}

/*
 * @see description on header file
 */
void SpecialRowRAM::close() {
}

/*
 * @see description on header file
 */
void SpecialRowRAM::truncateRow(int size) {
}

/*
 * @see description on header file
 */
void SpecialRowRAM::initialize(bool readOnly, int length) {
	if (row == NULL) {
		if (length == 0) {
			this->length = INITIAL_LENGTH;
		} else {
			this->length = length;
		}
		row = (cell_t*)malloc(length*sizeof(cell_t));
		if (row == NULL) {
			fprintf(stderr, "Out of memory (%d %d)\n", sum, length*sizeof(cell_t));
			exit(1);
		}
		sum += length;
	}

	//printf("%p: init(%d, %d) %d\n", row, 0, length, length);
}

/*
 * @see description on header file
 */
int SpecialRowRAM::write(const cell_t* buf, int offset, int len) {
	if (offset + len > length) {
		sum -= length;
		length *= LENGTH_MULTIPLIER;
		sum += length;
		row = (cell_t*)realloc(row, length*sizeof(cell_t));
		printf("%p: Up\n", row);
	}
	//printf("%p: Write(%d, %d) %d\n", row, offset, len, length);
	memcpy(row+offset, buf, len*sizeof(cell_t));
	return len;
}

/*
 * @see description on header file
 */
int SpecialRowRAM::read(cell_t* buf, int offset, int len) {
	memcpy(buf, row+offset, len*sizeof(cell_t));
	return len;
}


