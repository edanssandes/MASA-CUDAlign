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

#include "CPUBlockProcessor.hpp"

#include <stdio.h>

/*
 * Some macros
 */
#define MAX2(A,B) (((A)>(B))?(A):(B))
#define MAX3(A,B,C) (MAX2(MAX2((A),(B)),(C)))
#define MAX4(A,B,C,D) (MAX2(MAX2((A),(B)),MAX2((C),(D))))

#define DEBUG (0)

CPUBlockProcessor::CPUBlockProcessor() {
	this->seq0 = NULL;
	this->seq1 = NULL;
}

CPUBlockProcessor::~CPUBlockProcessor() {
	// TODO Auto-generated destructor stub
}

void CPUBlockProcessor::setSequences(const char* seq0, const char* seq1, int seq0_len, int seq1_len) {
	this->seq0 = seq0;
	this->seq1 = seq1;
}

void CPUBlockProcessor::unsetSequences() {

}


/**
 * Implements the smith waterman recurrence function.
 *
 * @param c0 char of sequence s0
 * @param c1 char of sequence s1
 * @param[in,out] 	e00 Input E[i][j-1]; Output E[i][j]
 * @param[in,out] 	f00 Input F[i-1][j]; Output F[i][j]
 * @param[in] 		h01	Input H[i][j-1]
 * @param[in] 		h11 Input H[i-1][j-1]
 * @param[in] 		h10 Input H[i-1][j]
 * @param[out] 		h00 Ouput H[i][j]
 */
static void sw(const unsigned char c0, const unsigned char c1,
			int *e00, int *f00, const int h01, const int h11, const int h10, int *h00) {
    *e00 = MAX2(h01-DNA_GAP_OPEN, *e00)-DNA_GAP_EXT; // Horizontal propagation
    *f00 = MAX2(h10-DNA_GAP_OPEN, *f00)-DNA_GAP_EXT; // Vertical propagation
    int v1 = h11+((c1!=c0)?DNA_MISMATCH:DNA_MATCH);
    *h00 = MAX4(0, v1, *e00, *f00);
}

/**
 * Implements the Needleman Wunsch recurrence function.
 *
 * @param c0 char of sequence s0
 * @param c1 char of sequence s1
 * @param[in,out] 	e00 Input E[i][j-1]; Output E[i][j]
 * @param[in,out] 	f00 Input F[i-1][j]; Output F[i][j]
 * @param[in] 		h01	Input H[i][j-1]
 * @param[in] 		h11 Input H[i-1][j-1]
 * @param[in] 		h10 Input H[i-1][j]
 * @param[out] 		h00 Ouput H[i][j]
 */
static void nw(const unsigned char c0, const unsigned char c1,
			int *e00, int *f00, const int h01, const int h11, const int h10, int *h00) {

    *e00 = MAX2(h01-DNA_GAP_OPEN, *e00)-DNA_GAP_EXT; // Horizontal propagation
    *f00 = MAX2(h10-DNA_GAP_OPEN, *f00)-DNA_GAP_EXT; // Vertical propagation
    int v1 = h11+((c1!=c0)?DNA_MISMATCH:DNA_MATCH);
    *h00 = MAX3(v1, *e00, *f00);
}

/**
 * Executes the SW/NW recurrence function for the given block.
 * @param[in,out] 	row		Input: cells from the first row of the block, where
 * 									 row[k] represents cell (i0-1,j0+k);
 * 							Output: the same range is used for write, but the output represents the
 * 									last row of the block;
 * @param[in,out] 	col		Input: cells from the first column of the partition, where
 * 									col[k] represents cell (i0-1+k,j0-1). Note that
 * 									the col vector contains the diag cell (i0-1,j0-1).
 * 							Output: the same idea, but representing the last column of the partition.
 * 							 		Note that col[0] represents the diag cell (i0-1,j0-1).
 *
 * @param[in] 		i0	start row
 * @param[in] 		j0	start column
 * @param[in] 		i1	end row
 * @param[in] 		j1	end column
 * @return
 */
score_t CPUBlockProcessor::processBlock(cell_t *row, cell_t *col,
		const int i0, const int j0, const int i1, const int j1,
		const int recurrenceType) {
	/* Initializing the best score of this block */
	score_t block_best;
	block_best.i = -1;
	block_best.j = -1;
	block_best.score = -INF;

	/* sequence strings starting from the position i0 and j0. */
	const char* seq0 = this->seq0 + i0;
	const char* seq1 = this->seq1 + j0;

	int h11 = col[0].h; // diagonal cell H[i-1][j-1]
	//printf("[%d..%d][%d..%d] %d\n", i0, i1, j0, j1, h11);

	for (int i=0; i<i1-i0; i++) {
		/* Reads cells from the previous left-block */
		int h01 = col[i+1].h;	// H[i][j-1]
		int e00 = col[i+1].e;	// E[i][j-1]

		const unsigned char c = seq0[i];
		for (int j=0; j<j1-j0; j++) {
			int h10 = row[j].h; // H[i-1][j]
			int f10 = row[j].f; // F[i-1][j]

			/* Calculates H[i][j] */
			int h00;
			if (recurrenceType == SMITH_WATERMAN) {
				sw(c, seq1[j], &e00, &f10, h01, h11, h10, &h00);
			} else {
				nw(c, seq1[j], &e00, &f10, h01, h11, h10, &h00);
			}

			/* Store the cells to be used in the next iteration */
			h11 = h10;
			h01 = h00;
			row[j].h = h00;
			row[j].f = f10;

			/* Updates best score if necessary */
			if (block_best.score < h00) {
				block_best.score = h00;
				block_best.i = i0+i;
				block_best.j = j0+j;
			}
		}

		/* Store cells to the next right block */
		if (i==0) {
			col[0].h = h11; // Last diagonal cell H[i-1][j-1]
		}
		h11 = col[i+1].h;
		col[i+1].h = h01;
		col[i+1].e = e00;
	}
	//printf("[%d..%d][%d..%d] %d\n", i0, i1, j0, j1, h11);

	if (DEBUG) printf("ProcessBlock (%d,%d)-(%d,%d) - best:(%d,%d,%d)\n", i0, j0, i1, j1, block_best.score, block_best.i, block_best.j);

	return block_best;
}

