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

#include "FirstRow.hpp"
#include "../../libmasa/libmasa.hpp"

#define DEBUG (0)

/*
 * @see description on header file
 */
FirstRow::FirstRow() {
//	score_params = NULL;
//	firstRowGapped = false;
	reader = NULL;
	setId(0);
}

/*
 * @see description on header file
 */
FirstRow::~FirstRow(){
}

/*
 * @see description on header file
 */
//void FirstRow::setParams(const score_params_t* score_params, bool firstRowGapped) {
//	this->score_params = score_params;
//	this->firstRowGapped = firstRowGapped;
//}


/*
 * @see description on header file
 */
void FirstRow::close() {
}

/*
 * @see description on header file
 */
void FirstRow::truncateRow(int size) {
}

/*
 * @see description on header file
 */
void FirstRow::initialize(bool readOnly, int length) {
}

/*
 * @see description on header file
 */
int FirstRow::write(const cell_t* buf, int offset, int len) {
	return 0;
}

void FirstRow::setCellsReader(SeekableCellsReader* reader) {
	this->reader = reader;
}

/*
 * @see description on header file
 */
int FirstRow::read(cell_t* buf, int offset, int len) {
	if (DEBUG) printf("FirstRow::read(%p, %d, %d)\n", buf, offset, len);
//	for (int k=0; k<len; k++) {
//		int ir = offset+k;
//		if (firstRowGapped) {
//			buf[k].h = (ir==0) ? 0 : -ir*score_params->gap_ext-score_params->gap_open;
//		} else {
//			buf[k].h = 0;
//		}
//		buf[k].f = -INF;
//	}
//	// we do not need to worry about the reverse vector, since the vector
//	// will be correctly reversed in the SpecialRow class.
//	return len;
	reader->seek(offset);
	return reader->read(buf, len);
}


