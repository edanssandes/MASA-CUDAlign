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

#include "AlignerUtils.hpp"

#include <stdlib.h>
#include <unistd.h>

#define MAX2(A,B) (((A)>(B))?(A):(B))

#define DEBUG (0)

/**
 * Splits the columns in n equal blocks.
 *
 * @param pos		A vector with (blocks+1) elements.
 * @param j0,j1		Range of the columns
 * @param blocks	Number of blocks to split.
 */
void AlignerUtils::splitBlocksEvenly(int* pos, int j0, int j1, int blocks) {
	int xLen = j1 - j0;
	int xMod = xLen % blocks;
	for (int i = 0; i < blocks; i++) {
		pos[i] = j0 + ((xLen / blocks) * i + (i < xMod ? i : xMod));
	}
	pos[blocks] = j1;
}




match_result_t AlignerUtils::matchColumn(const cell_t* buffer, const cell_t* base, int len, int goalScore, int gap_open_penalty) {
	//printf ( "BUSOUT[%d..%d](%d) goal: %d\n", i, i+len, len, goal );
	//cell_t* h_busOut = &col[i];
	//char* my_seq0 = getSeqVertical();
	//char* my_seq1 = getSeqHorizontal();
	bool end = false;
	match_result_t match_result;
	match_result.found = false;

	for ( int k = 0; k < len && !end; k++ ) {
		int sum_match = base[k].h + buffer[k].h;
		int sum_gap = base[k].e + buffer[k].e + gap_open_penalty;

		const char* result;
		if  (sum_match == goalScore) {
			result = "*Match";
			match_result.found = true;
			match_result.k = k;
			match_result.score = base[k].h;
			match_result.type = MATCH_ALIGNED;
			end = true;
		} else if (sum_gap == goalScore) {
			result = "*Gap";
			match_result.found = true;
			match_result.k = k;
			match_result.score = base[k].e;// + gap_open_penalty;
			match_result.type = MATCH_GAPPED;
			end = true;
		} else if (sum_match>goalScore || sum_gap>goalScore) {
			result = "*Error";
			match_result.type = sum_match > goalScore ? MATCH_ERROR_1 : MATCH_ERROR_2;
			end = true;
		} else {
			result = "";
		}

		if (DEBUG) {
			//char chunk0[6] = {};
			//char chunk1[6] = {};
			//strncpy(chunk0, &my_seq0[MAX2(my_i-5, 0)], my_i < 5 ? my_i : 5);
			//strncpy(chunk1, &my_seq1[MAX2(my_j-5, 0)], my_j < 5 ? my_j : 5);
			//if (debug) printf ( "i:%8d[%4d] SW: %4d/%4d  BUS_H: (%4d/%4d)  SUM: %4d/%4d%s\n",
			//printf ( "k:%4d %5s[%.1s]%.5s %5s[%.1s]%.5s SW: %4d/%4d  BUS_H: (%4d/%4d)  SUM: %4d/%4d%s\n",
			//chunk0, &my_seq0[my_i], &my_seq0[my_i + 1],
			//chunk1, &my_seq1[my_j], &my_seq1[my_j + 1],
			printf ( "k:%4d SW: %4d/%4d  BUS_H: (%4d/%4d)  SUM: %4d/%4d%s GOAL: %d\n", k,
					buffer[k].h, buffer[k].e,
					base[k].h, base[k].e,
					sum_match, sum_gap, result, goalScore);
		}
	}
	if (end && match_result.type < 0) {
		fflush(stdout);
		fprintf ( stderr, "ERROR: Backtrace lost (%d)! Can't continue.\n", match_result.type);
		_exit ( 1 );
	}
	return match_result;
}
