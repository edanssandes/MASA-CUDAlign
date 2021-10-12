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

#ifndef CPUBLOCKPROCESSOR_HPP_
#define CPUBLOCKPROCESSOR_HPP_

#include "AbstractBlockProcessor.hpp"
#include "../libmasa.hpp"

/*
 * The score constants
 */
#define DNA_MATCH       (1)
#define DNA_MISMATCH    (-3)
#define DNA_GAP_EXT     (2)
#define DNA_GAP_OPEN    (3)
#define DNA_GAP_FIRST   (DNA_GAP_EXT+DNA_GAP_OPEN)



class CPUBlockProcessor: public AbstractBlockProcessor {
public:

	CPUBlockProcessor();
	virtual ~CPUBlockProcessor();

	virtual void setSequences(const char* seq0, const char* seq1, int seq0_len, int seq1_len);
	virtual void unsetSequences();
	virtual score_t processBlock(cell_t *row, cell_t *col, const int i0, const int j0, const int i1, const int j1, const int recurrenceType);

private:
	const char *seq0;
	const char *seq1;
};

#endif /* CPUBLOCKPROCESSOR_HPP_ */
