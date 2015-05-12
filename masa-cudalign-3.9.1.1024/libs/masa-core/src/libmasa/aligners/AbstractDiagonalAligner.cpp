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

#include "AbstractDiagonalAligner.hpp"

#include <stdlib.h>

/**
 * Set to (1) in order to print debug information in the stdout. This
 * significantly degrades the performance.
 */
#define DEBUG (0)

/**
 * Defines a lower-bound flush interval between two special rows.
 */
#define MINIMUM_FLUSH_INTERVAL	(8*1024)

/**
 * AbstractDiagonalAligner constructor.
 */
AbstractDiagonalAligner::AbstractDiagonalAligner() {
	pruner = new BlockPruningDiagonal();

}

/**
 * AbstractDiagonalAligner destructor.
 */
AbstractDiagonalAligner::~AbstractDiagonalAligner() {

}


/**
 * Aligns the given partition, processing it by antidiagonals.
 *
 * @param partition partition to be aligned.
 * @see IAligner::alignPartition
 */
void AbstractDiagonalAligner::alignPartition(Partition partition) {
	this->partition = partition;

	//createDispatcherQueue();
	prepareIterations();
	while (hasMoreIterations() && mustContinue()) {
		processNextIteration();
	}
	finalizeIterations();
	if (DEBUG) printf("End of alignPartition: %d %d\n", hasMoreIterations(), mustContinue());
	//destroyDispatcherQueue();
}

/**
 * Configures the grid and initializes other structures before the beginning of
 * the diagonal iterations.
 */
void AbstractDiagonalAligner::prepareIterations() {
	Grid* grid = configureGrid(partition);

	initializeBlockPruning(pruner);

	h_loadColumn = (cell_t*)malloc((getBlockHeight()+1)*sizeof(cell_t));

	/* reads the first top-left cell of the partition. */
	cell_t dummy;
	receiveFirstColumn(&dummy, 1); // initializes the AbstractAligner::firstColumnTail
	receiveFirstRow(&dummy, 1); // initializes the AbstractAligner::firstRowTail
	// TODO assert if both tails are equal?

	loadFirstRow();


	currentExternalDiagonal = 0;

	grid->getGridHeight();
	gridHeight = partition.getHeight()/getBlockHeight() + 1;
    externalDiagonalCount = gridHeight + gridWidth;
    if (DEBUG) printf("Grid height: %d    Blocks: %d    %d-%d:%d\n", gridHeight, gridWidth, partition.getI1(), partition.getI0(), partition.getHeight());
    if (DEBUG) printf("START: %d steps\n", externalDiagonalCount);

	windowStart = 0;
	windowEnd = gridWidth;

    initializeDiagonals(); // calls subclass
}

/**
 * Processes the next diagonal and loads/flushes the rows and columns necessary
 * for the MASA-Core integration.
 */
void AbstractDiagonalAligner::processNextIteration() {

	/* Implemented capability: aligner_capabilities_t::customize_first_column */
	if (getFirstColumnInitType() != INIT_WITH_ZEROES) {
		loadFirstColumn();
	}

	processDiagonal(currentExternalDiagonal, windowStart, windowEnd);

	/* Implemented aligner_capabilities_t::dispatch_special_row */
	if (mustDispatchSpecialRows()) {
		flushSpecialRows();
	}
	/* Implemented aligner_capabilities_t::dispatch_last_row */
	if (mustDispatchLastRow()) {
		flushLastRow();
	}
	/* Implemented aligner_capabilities_t::dispatch_last_rcolumn */
	if (mustDispatchLastColumn()) {
		flushLastColumn();
	}
	/* Implemented aligner_capabilities_t::dispatch_scores */
	if (mustDispatchScores()) {
		flushBlockScores();
	}
	/* Implemented aligner_capabilities_t::block_pruning */
	if (mustPruneBlocks()) {
		pruneBlocks();
	}
	/* Implemented aligner_capabilities_t::dispatch_last_cell */
	if (mustDispatchLastCell()) {
		flushLastCell();
	}

	/* Statistics */
	int b0 = max(0, currentExternalDiagonal	- (externalDiagonalCount - gridWidth));
	int b1 = min(currentExternalDiagonal, gridWidth);
	int jb0;
	int jb1;
	statTotalBlocks += b1 - b0 + 1;
	statPrunedBlocksLeft += max(windowStart - b0, 0);
	statPrunedBlocksRight += max(b1 - windowEnd - 1, 0);
	getGrid()->getBlockPosition(b0  , 0, NULL, &jb0, NULL, NULL);
	getGrid()->getBlockPosition(b1-1, 0, NULL, NULL, NULL, &jb1);
	if (jb1 > 0) {
		statTotalCells += ((long long)jb1-jb0)* getBlockHeight();
	}

	currentExternalDiagonal++;
}

/**
 * Tests if we have more diagonals to be processed.
 * @return true if we have more diagonals.
 */
bool AbstractDiagonalAligner::hasMoreIterations() {
	return currentExternalDiagonal < externalDiagonalCount;
}

/**
 * Finalizes the execution of the partition.
 */
void AbstractDiagonalAligner::finalizeIterations() {
	finalizeDiagonals(); // calls subclass

	delete h_loadColumn;
	h_loadColumn = NULL;
}

/**
 * Configures the grid for the given partition.
 * @param partition partition to be aligned.
 */
Grid* AbstractDiagonalAligner::configureGrid(Partition partition) {
	/* creates the grid */
	Grid* grid = this->createGrid(partition);

	this->gridWidth = getGridWidth(partition.getWidth());
	grid->setBlockHeight(getBlockHeight());
	grid->splitGridHorizontally(gridWidth);

	/* grid statistics */
	statMinGridWidth  = std::min(statMinGridWidth , gridWidth);
	statMaxGridWidth  = std::max(statMaxGridWidth , gridWidth);

	if (DEBUG) printf("Configured Grid width: %d\n", gridWidth);
	return grid;
}


/**
 * @copydoc IAligner::clearStatistics
 */
void AbstractDiagonalAligner::clearStatistics() {
	statPrunedBlocksLeft = 0;
	statPrunedBlocksRight = 0;
	statTotalBlocks = 0;
	statTotalCells = 0;

	statMinGridWidth = INF;
	statMaxGridWidth = 0;
}

/** Empty stub for the superclass virtual method.
 * @copydoc IAligner::clearStatistics
 */
void AbstractDiagonalAligner::printInitialStatistics(FILE* file) {
}

/** Empty stub for the superclass virtual method.
 * @copydoc IAligner::printStageStatistics
 */
void AbstractDiagonalAligner::printStageStatistics(FILE* file) {
}

/** Empty stub for the superclass virtual method.
 * @copydoc IAligner::printFinalStatistics
 */
void AbstractDiagonalAligner::printFinalStatistics(FILE* file) {
}

/**
 * Prints the pruning statistics and grid width used range.
 * @param file handler to print out the statistics.
 * @see IAligner::printStatistics
 */
void AbstractDiagonalAligner::printStatistics(FILE* file) {

	fprintf(file, "\n=====  PRUNING STATS   =====\n");
	fprintf(file, "Pruned Blocks: %d = %d + %d\n",
			statPrunedBlocksLeft+statPrunedBlocksRight,
			statPrunedBlocksLeft,
			statPrunedBlocksRight);
	fprintf(file, "Pruned Blocks: %.4f = %.4f + %.4f\n",
			((statPrunedBlocksLeft+statPrunedBlocksRight) * 100.0f) / statTotalBlocks,
			(statPrunedBlocksLeft * 100.0f) / statTotalBlocks,
			(statPrunedBlocksRight * 100.0f) / statTotalBlocks);

	fprintf(file, "\n===== RUNTIME VARIABLES =====\n");
	fprintf(file, "     Block Count: %d-%d\n", statMinGridWidth, statMaxGridWidth);

	fflush(file);
}

/**
 * Returns the number of processed cells since the last call to
 * AbstractDiagonalAligner::clearStatistics method.
 *
 * @return number of processed cells
 */
long long AbstractDiagonalAligner::getProcessedCells() {
	return statTotalCells;
}

/**
 * MASA-Core prints this string periodically.
 * @return the string to be printed.
 */
const char* AbstractDiagonalAligner::getProgressString() const {
	static char str[128];

	sprintf(str, "PROGRESS: %4d/%4d  cut:%d/%d (%d/%d %.1f%% %.1f%%)",
			currentExternalDiagonal, externalDiagonalCount,
			windowStart, windowEnd,
			this->statPrunedBlocksLeft, this->statPrunedBlocksRight,
			(this->statPrunedBlocksLeft * 100.0f) / statTotalBlocks,
		(this->statPrunedBlocksRight * 100.0f) / statTotalBlocks);
	return str;
}

/**
 * Iterates on the blocks and check if any of them must flush its last row
 * in the disk. If so, the row is copied from the aligner and dispatched to the
 * MASA-Core. Note that the blocks are disposed in a diagonal, so there may
 * be a maximum of one block per special row.
 */
void AbstractDiagonalAligner::flushSpecialRows() {
	if (getSpecialRowInterval() > 0) {
		const int blockHeight = getBlockHeight();

        if (isSpecialRow(currentExternalDiagonal+1)) {
        	/*
        	 * Dispatches the first cell of the row, that is copied from
        	 * the first column (firstColumnTail).
        	 */
        	cell_t first_cell = getFirstColumnTail();
        	first_cell.f = -INF;
        	dispatchRow(partition.getI0()+(currentExternalDiagonal+1)*blockHeight, &first_cell, 1);
        }

		for (int k = 0; k < gridWidth && k <= currentExternalDiagonal; k++) {
			int bx = k;
			int by = currentExternalDiagonal - bx;

			if (isSpecialRow(by)) { // Check if this block must be flushed
				int i = by * blockHeight;
				int x0;
				int x1;
				getGrid()->getBlockPosition(bx, 0, NULL, &x0, NULL, &x1);
				int xLen = x1 - x0;

				const cell_t* specialRow = getSpecialRow(x0, xLen); // from subclass
				dispatchRow(partition.getI0() + i, specialRow, xLen);
			}

		}
	}
}

/**
 * This function reads a chunk of the last row from the aligner and dispatch
 * it to the MASA-Core. This method is called multiple times during the
 * last external diagonals, since each of the bottom-most blocks
 * calculates a range of the last row.
 */
void AbstractDiagonalAligner::flushLastRow() {
	if (currentExternalDiagonal >= gridHeight-1) {

		for (int k = 1; k <= gridWidth && k <= currentExternalDiagonal; k++) {
			int bx = k;
			const int by = currentExternalDiagonal - bx;

			bool flushThisRow = (by == gridHeight-1);

			if (flushThisRow) {
				int x0;
				int x1;
				getGrid()->getBlockPosition(bx-1, 0, NULL, &x0, NULL, &x1);
				int xLen = x1 - x0;
				const cell_t* lastRow = getLastRow(x0, xLen); // from subclass
				if (DEBUG) printf("dispatchRow: %d..%d %d %d\n", x0, x1, xLen, bx);
				if (bx == 1) {
					cell_t first_cell = getFirstColumnTail();
					first_cell.f = -INF;
			    	dispatchRow(partition.getI1(), &first_cell, 1);
				}
				dispatchRow(partition.getI1(), lastRow, xLen);
			}

		}
	}
}

/**
 * This function reads a chunk of the last column from the aligner and dispatch
 * it to the MASA-Core. This method is called once for each external diagonal,
 * since the last block is always calculating a new chunk of the last column.
 */
void AbstractDiagonalAligner::flushLastColumn() {
	const int blockHeight = getBlockHeight();
	int i = partition.getI0() + (currentExternalDiagonal-gridWidth)*blockHeight;
	if (i >= partition.getI0()) {
		int len = getBlockHeight();
		if (i+len > partition.getI1()) {
			len = partition.getI1()-i;
		}
		const cell_t* column_chunk = getLastColumn(i, len); // from subclass
		dispatchColumn(partition.getJ1(), column_chunk, len);
	}
}

/**
 * This function reads the bottom-left cell from aligner and dispatch it
 * to the MASA-Core.
 */
void AbstractDiagonalAligner::flushLastCell() {
	if (currentExternalDiagonal+1 >= externalDiagonalCount) { // last iteration
		const cell_t* lastCell = getLastRow(partition.getJ1()-1, 1); // from subclass

		score_t score;
		score.i = partition.getI1()-1;
		score.j = partition.getJ1()-1;
		score.score = lastCell->h;

		dispatchScore(score);
	}
}

/**
 * This function reads the block scores from aligner and dispatch them
 * to the MASA-Core.
 */
void AbstractDiagonalAligner::flushBlockScores() {
	const score_t* scores = getBlockScores(); // from subclass
	for (int bl=0; bl<gridWidth; bl++) {
		int x = bl;
		int y = currentExternalDiagonal - bl;

		if (y >= 0 && y < externalDiagonalCount) {
			// Dispatch scores for each valid block
			dispatchScore(scores[bl], x, y);
		}
	}
}

/**
 * Initialize the first row of the aligner with the cells received
 * from the MASA-Core. The first row is read all at once.
 */
void AbstractDiagonalAligner::loadFirstRow() {
	// The seq1_offset allow us to reduce the memory usage.
	int j0 = partition.getJ0();
	int j1 = partition.getJ1();

	int chunk_size = j1+10; // TODO optimize
	cell_t* chunk = (cell_t*)malloc(chunk_size*sizeof(cell_t));

	receiveFirstRow((cell_t*) ((chunk + j0)), j1 - j0); // from MASA-Core

	// The 1st cell of the last column is the last cell of the first row.
	cell_t first_cell = getFirstRowTail();
	first_cell.f = -INF;
	dispatchColumn(partition.getJ1(), &first_cell, 1);

	setFirstRow((const cell_t*)&chunk[j0], j0, j1-j0); // to subclass
	delete chunk;
}

/**
 * Initialize the first column of the aligner with the cells received
 * from the MASA-Core. The first column is read in
 * chunks, which size is defined by the getBlockHeight() function.
 */
void AbstractDiagonalAligner::loadFirstColumn() {
	int pos_i = ((currentExternalDiagonal)*getBlockHeight());
    if (pos_i < partition.getHeight()) {
        int len = getBlockHeight(); // from subclass
        if (pos_i + len >= partition.getHeight()) {
        	len = partition.getHeight() - pos_i;
        }

        /* Puts the H[i-1][j-1] dependency in the first cell of the column. */
        h_loadColumn[0] = getFirstColumnTail();

        /* Receives the first column chunk from the MASA-core */
        receiveFirstColumn((cell_t*) (h_loadColumn+1), len); // from MASA-Core

		/* Padding */
		for (int i = len; i < getBlockHeight(); i++) {
			h_loadColumn[i].h = -INF;
			h_loadColumn[i].e = -INF;
		}

		/* Updates the first column in GPU */
		setFirstColumn(h_loadColumn, pos_i, len); // to subclass
    }
}


/**
 * Defines if the blocks in row $by$ must flush their last row. The top-most
 * and bottom-most rows are never special rows.
 *
 * @param by the block's row id
 * @return true if the blocks inr row $by$ must save their last row.
 */
bool AbstractDiagonalAligner::isSpecialRow(int by) {
	const int block_height = getBlockHeight();
	int flush_block_interval = (getSpecialRowInterval()+block_height-1)/block_height;
	if (flush_block_interval <= 0) {
		flush_block_interval = 1;
	}
	if (flush_block_interval <= MINIMUM_FLUSH_INTERVAL/block_height) {
		flush_block_interval = MINIMUM_FLUSH_INTERVAL/block_height;
	}

	int i = by * block_height;
	return ((by % flush_block_interval == 0) && (i > 0 && i < partition.getHeight()) );
}

/**
 * Returns the partition currently being processed.
 * @return the partition currently being processed.
 */
Partition AbstractDiagonalAligner::getPartition() const {
	return partition;
}

/**
 * Updates the pruning window accordingly to the last block scores.
 */
void AbstractDiagonalAligner::pruneBlocks() {
	const score_t* block_scores = getBlockScores();
	int prevStart;
	int prevEnd;
	pruner->getNonPrunableWindow(&prevStart, &prevEnd);
	pruner->updatePruningWindow(currentExternalDiagonal-1, block_scores);
	pruner->getNonPrunableWindow(&windowStart, &windowEnd);
	if (windowEnd < prevEnd) {
		clearPrunedBlocks(windowEnd, prevEnd);
	}
}



