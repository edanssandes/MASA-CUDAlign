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

#include "AbstractBlockAligner.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "../processors/CPUBlockProcessor.hpp"

/**
 * Set to (1) in order to print debug information in the stdout. This
 * significantly degrades the performance.
 */
#define DEBUG (0)

/*
 * The score constants
 */
#define DNA_MATCH       (1)
#define DNA_MISMATCH    (-3)
#define DNA_GAP_EXT     (2)
#define DNA_GAP_OPEN    (3)
#define DNA_GAP_FIRST   (DNA_GAP_EXT+DNA_GAP_OPEN)


#ifdef ENABLE_PROFILING
#include "../libs/masa-core/src/common/Timer.hpp"
static FILE* bitmap = NULL;
#define PROFILING_INIT(name) bitmap = fopen(name, "wt");
#define PROFILING_TIME(var) float var = Timer::getGlobalTime();
#define PROFILING_PRINT(bx, by, score, done, time) fprintf(bitmap, "%d %d %d %d %d %p\n", by, bx, score, done, (int)((time)*1000), pthread_self());
#else
#define PROFILING_INIT
#define PROFILING_TIME(var)
#define PROFILING_PRINT(bx, by, score, done, time)
#endif

/**
 * Maximum recommended block size for better performance
 */
#define RECOMMENDED_BLOCK_SIZE	(1024)

/**
 * Minimum recommended grid size for better performance
 */
#define RECOMMENDED_GRID_SIZE	(8)


/**
 * AbstractBlockAligner Constructor.
 */
AbstractBlockAligner::AbstractBlockAligner(AbstractBlockProcessor* blockProcessor, BlockAlignerParameters* params) {
	/*
	 * Initializations
	 */
	col = NULL;
	row = NULL;

	/*
	 *  defines the constant parameters to be returned in the
	 *  getScoreParameters() method
	 */
	score_params.match = DNA_MATCH;
	score_params.mismatch = DNA_MISMATCH;
	score_params.gap_open = DNA_GAP_OPEN;
	score_params.gap_ext = DNA_GAP_EXT;

	/*
	 * creates the custom parameters
	 */
	if (params == NULL) {
		this->params = new BlockAlignerParameters();
	} else {
		this->params = params;
	}
	if (blockProcessor == NULL) {
		this->blockProcessor = new CPUBlockProcessor();
	} else {
		this->blockProcessor = blockProcessor;
	}
	this->blockPruner = new BlockPruningGenericN2();

	/*
	 * Must be called to enable --fork parameter. This will allow
	 * 2 process for each CPU. Each process will receive the same
	 * workload.
	 */
	int numCPU = sysconf( _SC_NPROCESSORS_ONLN );
	setForkCount(numCPU*2);

	preferredBlockSize = RECOMMENDED_BLOCK_SIZE;
	preferredGridSize = RECOMMENDED_GRID_SIZE;
}

/**
 * AbstractBlockAligner destructor.
 */
AbstractBlockAligner::~AbstractBlockAligner() {
}

/*
 * This method defines which capabilities are implemented by this aligner.
 * The dispatch_special_column and variable_penalties capabilities are
 * not used yet by the MASA framework. The block_pruning and dispatch_last_cell
 * capabilities are not supported yet by this example aligner.
 */
aligner_capabilities_t AbstractBlockAligner::getCapabilities() {
	aligner_capabilities_t capabilities;

	/* This is the supported capabilities of this Aligner subclass */
	capabilities.smith_waterman 			= SUPPORTED;
	capabilities.needleman_wunsch 			= SUPPORTED;
	capabilities.block_pruning 				= SUPPORTED;
	capabilities.customize_first_column 	= SUPPORTED;
	capabilities.customize_first_row 		= SUPPORTED;
	capabilities.dispatch_last_cell 		= NOT_SUPPORTED;
	capabilities.dispatch_last_column 		= SUPPORTED;
	capabilities.dispatch_last_row 			= SUPPORTED;
	capabilities.dispatch_special_column 	= SUPPORTED;
	capabilities.dispatch_special_row 		= SUPPORTED;
	capabilities.dispatch_block_scores		= SUPPORTED;
	capabilities.dispatch_scores			= SUPPORTED;
	capabilities.process_partition 			= SUPPORTED;
	capabilities.variable_penalties 		= NOT_SUPPORTED;
	capabilities.fork_processes				= SUPPORTED;

	capabilities.maximum_seq0_len	= 0;
	capabilities.maximum_seq1_len	= 0;

	return capabilities;
}

/*
 * Returns the constant match/mismatch/gaps scores.
 */
const score_params_t* AbstractBlockAligner::getScoreParameters() {
	return &score_params;
}

/*
 * See MicParameters and AbstractAlignerParameters classes.
 */
IAlignerParameters* AbstractBlockAligner::getParameters() {
	return this->params;
}

/*
 * Initializes some structures of the Aligner. This method
 * is called only once for each stage, and only the sequence data may be
 * obtained in this time. The parameters must also be used here.
 * The partition boundaries must only be used in the alignPartition() method.
 *
 * See the IAligner and AbstractAligner classes documentation.
 */
void AbstractBlockAligner::initialize() {

}

/*
 * The finalize method is called once for each stage, right before
 * the printFinalStatistics. Here we must deallocate all the previously
 * allocated structure.
 *
 * See the IAligner and AbstractAligner classes documentation.
 */
void AbstractBlockAligner::finalize() {
	if (getGrid() != NULL) {
		deallocateStructures();
	}
}


/**
 * Configures the grid for the given partition.
 * @param partition partition to be aligned.
 */
Grid* AbstractBlockAligner::configureGrid(Partition partition) {
	/* creates the grid */
	Grid* grid = this->createGrid(partition);

	grid->setMinBlockSize(MIN_BLOCK_SIZE, MIN_BLOCK_SIZE);

	/* Block/Grid height */
	if (params->getBlockHeight() > 0) {
		grid->setBlockHeight(params->getBlockHeight());
	} else if (params->getGridHeight() > 0) {
		grid->splitGridVertically(params->getGridHeight());
	} else {
		// Automatic configuration
		if (!mustDispatchLastColumn()) {
			int height = preferredBlockSize;
			if (grid->getHeight() / height < preferredGridSize) {
				height = grid->getHeight() / preferredGridSize;
			}
			grid->setBlockHeight(height);
		} else {
			grid->setBlockHeight(grid->getWidth()/preferredGridSize/2);
		}
	}

	/* Block/Grid width */
	if (params->getBlockWidth() > 0) {
		grid->setBlockWidth(params->getBlockWidth());
	} else if (params->getGridWidth() > 0) {
		grid->splitGridHorizontally(params->getGridWidth());
	} else {
		// Automatic configuration
		if (!mustDispatchLastColumn()) {
			int width = preferredBlockSize;
			if (grid->getWidth() / width < preferredGridSize) {
				width = grid->getWidth() / preferredGridSize;
			}
			grid->setBlockWidth(width);
		} else {
			grid->splitGridHorizontally(preferredGridSize);
		}
	}

	/* block statistics */
	int block_width  = grid->getBlockWidth(0,0);
	int block_height = grid->getBlockHeight(0,0);
	statMinBlockWidth  = std::min(statMinBlockWidth , block_width);
	statMinBlockHeight = std::min(statMinBlockHeight, block_height);
	statMaxBlockWidth  = std::max(statMaxBlockWidth , block_width);
	statMaxBlockHeight = std::max(statMaxBlockHeight, block_height);

	/* grid statistics */
	int grid_width = grid->getGridWidth();
	int grid_height = grid->getGridHeight();
	statMinGridWidth  = std::min(statMinGridWidth , grid_width);
	statMinGridHeight = std::min(statMinGridHeight, grid_height);
	statMaxGridWidth  = std::max(statMaxGridWidth , grid_width);
	statMaxGridHeight = std::max(statMaxGridHeight, grid_height);

	if (DEBUG) printf("Configured Grid: B: %d x %d    G: %d x %d\n", block_width, block_height, grid_width, grid_height);
	return grid;
}


void AbstractBlockAligner::setSequences(const char* seq0, const char* seq1, int seq0_len, int seq1_len) {
	blockProcessor->setSequences(seq0, seq1, seq0_len, seq1_len);
}

void AbstractBlockAligner::unsetSequences() {
	blockProcessor->unsetSequences();
}

/**
 * Aligns the given partition, processing it block by block using a customized
 * scheduler.
 *
 * @param partition partition to be aligned.
 * @see IAligner::alignPartition
 */
void AbstractBlockAligner::alignPartition(Partition partition) {
	//fprintf(stderr, "alignPartition()\n");
	//createDispatcherQueue(); // Uncomment this if you want thread-safe calls

	/* configures the grid */
	Grid* grid = configureGrid(partition);

	/* the block pruning initialization must be done after grid configuration */
	initializeBlockPruning(blockPruner);

	/* allocates the memory structures. It must be called after grid configuration */
	allocateStructures();

	/* reads the first top-left cell of the partition. */
	cell_t dummy;
	receiveFirstColumn(&dummy, 1); // initializes the AbstractAligner::firstColumnTail
	receiveFirstRow(&dummy, 1); // initializes the AbstractAligner::firstRowTail
	// TODO assert if both tails are equal?

	/* statistics initializations */
	statTotalBlocks = 0;
	statPrunedBlocks = 0;

	static char str[500];
	sprintf(str, "profiling.%08d.%08d.%08d.%08d.%d.txt", partition.getI0(), partition.getJ0(), partition.getI1(), partition.getJ1(), mustDispatchLastColumn());
	PROFILING_INIT(str);

	/* local initializations */
	int grid_width = grid->getGridWidth();
	int grid_height = grid->getGridHeight();
	scheduleBlocks(grid_width, grid_height);

	// TODO o dispatch score deve ser feito on-the-fly durante o estagio 2,
	// caso contrario o processamento nao ira parar se encontrarmos o goal score.
	for (int bx = 0; bx < grid_width; bx++) {
		for (int by = 0; by < grid_height; by++) {
			/* Dispatch the best score found in block (bx,by) */
			dispatchScore(grid_scores[bx][by], bx, by);
		}
	}

	if (mustDispatchLastCell()) {
		score_t score;
		score.score = row[grid_width-1][grid->getBlockWidth(grid_width-1, grid_height-1)-1].h;
		score.i = partition.getI1()-1;
		score.j = partition.getJ1()-1;
		dispatchScore(score, grid_width-1, grid_height-1);
	}

	deallocateStructures();
	//destroyDispatcherQueue(); // Uncomment this if you want thread-safe calls
}


/**
 * This method aligns block (bx,by). This method calls the
 * AbstractBlockAligner::alignBlock(int,int,int,int,int,int)
 * with additional information about block coordinates.
 *
 * @param bx horizontal coordinate of the block in the grid
 * @param by vertical coordinate of the block in the grid
 */
void AbstractBlockAligner::alignBlock(int bx, int by) {
	int i0;
	int i1;
	int j0;
	int j1;

	/* obtains the boundaries of block (bx, by) */
	getGrid()->getBlockPosition(bx, by, &i0, &j0, &i1, &j1);

	alignBlock(bx, by, i0, j0, i1, j1);
}

/**
 * This method calls AbstractBlockProcessor::processBlock(int,int,int,int,int,int,int)
 * to execute the recurrence relation for a block.
 *
 * @param bx horizontal block coordinate
 * @param by vertical block coordinate
 * @param i0 vertical first row of the block
 * @param j0 horizontal first column of the block
 * @param i1 vertical last row of the block
 * @param j1 horizontal last column of the block
 * @param true if the block was processed or false if it was pruned.
 */
bool AbstractBlockAligner::processBlock(int bx, int by, int i0, int j0, int i1,	int j1) {
	if (!isBlockPruned(bx, by)) {
		/* the block was not pruned */
		if (DEBUG) printf(">>>AbstractBlockAligner::processBlock(%d, %d, %d, %d, %d, %d)\n", bx, by, i0, j0, i1, j1);


		//if (!mustContinue()) return; // MASA-core is telling to stop

		PROFILING_TIME(t0);

		/* processes the block */
		grid_scores[bx][by] = blockProcessor->processBlock(row[bx], col[by], i0, j0, i1, j1, getRecurrenceType());

		PROFILING_TIME(t1);
		PROFILING_PRINT(bx, by, grid_scores[bx][by].score, 1, t1-t0);

		/* Updates the block pruning status */
		pruningUpdate(bx, by, grid_scores[bx][by].score);

		increaseBlockStat(false);

		/* Dispatch the best score found in block (bx,by) */
		//dispatchScore(grid_scores[bx][by], bx, by);
		return true;
	} else {
		/* the block was pruned */
		ignoreBlock(bx, by);
		return false;
	}

}

/*
 * Profiles this block as "pruned"
 */
void AbstractBlockAligner::ignoreBlock(int bx, int by) {
	PROFILING_PRINT(bx, by, 0, 0, 0);
	increaseBlockStat(true);
}

/*
 * Updates statTotalBlocks and statPrunedBlocks variables
 */
void AbstractBlockAligner::increaseBlockStat(const bool pruned) {
	statTotalBlocks++;
	if (pruned) {
		statPrunedBlocks++;
	}
}


/**
 * Indicates if the blocks on row $by$ must dispatch its last row.
 *
 * @param by the row of blocks.
 * @return true if the last row of the block will be stored (special row).
 */
bool AbstractBlockAligner::isSpecialRow(int by) {
	/*
	 * Dispatch special rows with a minimum defined distance (getSpecialRowInterval()).
	 */
	if (mustDispatchLastRow() && by == getGrid()->getGridHeight()-1) {
		return true;
	} else if (mustDispatchSpecialRows()) {
		/*
		 * Note that only the last rows of the blocks are suitable to be a
		 * special row.
		 */
		const int block_height = getGrid()->getBlockHeight(0, 0); // considering that all the blocks has the same height
		int flush_block_interval = (getSpecialRowInterval()+block_height-1)/block_height;
		if (flush_block_interval <= 0) {
			flush_block_interval = 1;
		}

		return ((by+1) % flush_block_interval == 0);
	} else {
		return false;
	}
}

/**
 * Indicates if the blocks on column $bx$ must dispatch its last column.
 *
 * @param bx the column of blocks.
 * @return true if the last column of the block will be stored (special column).
 */
bool AbstractBlockAligner::isSpecialColumn(int bx) {
	return (mustDispatchLastColumn() && bx == getGrid()->getGridWidth()-1);
}

/**
 * Prints the pruning statistics and grid used range.
 * @param file handler to print out the statistics.
 * @see IAligner::printStatistics
 */
void AbstractBlockAligner::printStatistics(FILE* file) {
	// Print final statistics to the statistics file;
	fprintf(file, "\n=====  PRUNING STATS   =====\n");
	fprintf(file, "Pruned Blocks: %d\n", statPrunedBlocks);
	fprintf(file, "Pruned Blocks: %.4f%%\n",
			(statPrunedBlocks * 100.0f) / statTotalBlocks);

	fprintf(file, "\n===== RUNTIME VARIABLES =====\n");
	fprintf(file, "      Block Width: %d-%d\n", statMinBlockWidth, statMaxBlockWidth);
	fprintf(file, "     Block Height: %d-%d\n", statMinBlockHeight, statMaxBlockHeight);
	fprintf(file, "       Grid Width: %d-%d\n", statMinGridWidth, statMaxGridWidth);
	fprintf(file, "      Grid Height: %d-%d\n", statMinGridHeight, statMaxGridHeight);

	fflush(file);
}

/**
 * Prints the runtime parameters.
 * @param file handler to print out the statistics.
 * @see IAligner::printInitialStatistics
 */
void AbstractBlockAligner::printInitialStatistics(FILE* file) {
	fprintf(file, "\n===== RUNTIME PARAMETERS =====\n");
	fprintf(file, " %s height: %d\n",
			params->getBlockHeight() ? "Block" : "Grid",
			params->getBlockHeight() ? params->getBlockHeight() : params->getGridHeight());
	fprintf(file, " %s width: %d\n",
			params->getBlockWidth() ? "Block" : "Grid",
			params->getBlockWidth() ? params->getBlockWidth() : params->getGridWidth());
	fprintf(file, " ForkID: %d\n", params->getForkId());
}

/**
 * @copydoc IAligner::clearStatistics
 */
void AbstractBlockAligner::clearStatistics() {
	statTotalBlocks = 0;
	statPrunedBlocks = 0;

	statMinBlockWidth = INF;
	statMaxBlockWidth = 0;
	statMinBlockHeight = INF;
	statMaxBlockHeight = 0;

	statMinGridWidth = INF;
	statMaxGridWidth = 0;
	statMinGridHeight = INF;
	statMaxGridHeight = 0;
}

/** Empty stub for the superclass virtual method.
 * @copydoc IAligner::printStageStatistics
 */
void AbstractBlockAligner::printStageStatistics(FILE* file) {
}

/** Empty stub for the superclass virtual method.
 * @copydoc IAligner::printFinalStatistics
 */
void AbstractBlockAligner::printFinalStatistics(FILE* file) {
}

/*
 * This method returns a progress string that is printed periodically.
 */
const char* AbstractBlockAligner::getProgressString() const {
	return "";
}

/*
 *
 */
long long AbstractBlockAligner::getProcessedCells() {
	return 0; // Used to calculate the MCUPS performance metric
}

/**
 * Allocate vectors after sequence is set.
 */
void AbstractBlockAligner::allocateStructures() {
	/*
	 * Allocates the first row of each block. This vector is transferred
	 * from on block to the other, in the vertical direction.
	 */
	int grid_width = getGrid()->getGridWidth();
	row = new cell_t*[grid_width];
	for (int j=0; j<grid_width; j++) {
		int block_width = getGrid()->getBlockWidth(j,0);
		row[j] = new cell_t[block_width];
	}

	/*
	 * Allocates the first column of each block. This vector is transferred
	 * from on block to the other, in the horizontal direction.
	 */
	int grid_height = getGrid()->getGridHeight();
	col = new cell_t*[grid_height];
	for (int i=0; i<grid_height; i++) {
		int block_height = getGrid()->getBlockHeight(0,i);
		col[i] = new cell_t[block_height+1];
	}



	grid_scores = new score_t*[ grid_width ];
	for( int j = 0; j < grid_width; ++j ){
		grid_scores[j] = new score_t[ grid_height ];
		for( int i = 0; i < grid_height; ++i ) {
			grid_scores[j][i].score = -INF;
		}
	}
}

/**
 * Deallocate vectors after sequence is unset.
 */
void AbstractBlockAligner::deallocateStructures() {
	if (getGrid() == NULL) {
		return;
	}
	int grid_width = getGrid()->getGridWidth();
	int grid_height = getGrid()->getGridHeight();

	if (row != NULL) {
		for (int j = 0; j < grid_width; ++j) {
			delete[] row[j];
		}
		delete[] row;
		row = NULL;
	}
	if (col != NULL) {
		for (int i = 0; i < grid_height; ++i) {
			delete[] col[i];
		}
		delete[] col;
		col = NULL;
	}
	if(grid_scores != NULL) {
		for( int j = 0; j < grid_width; ++j ){
			delete[] grid_scores[j];
		}
		delete[] grid_scores;
		grid_scores = NULL;
	}
}

/**
 * Returns true if block (bx, by) can be pruned.
 *
 * @param bx horizontal block coordinate
 * @param by vertical block coordinate
 * @return pruned status of the block.
 */
bool AbstractBlockAligner::isBlockPruned(int bx, int by) const {
	return (blockPruner != NULL && blockPruner->isBlockPruned(bx, by));
}

/**
 * Updates the pruning status accordingly to the block score.
 *
 * @param bx horizontal block coordinate
 * @param by vertical block coordinate
 * @param score the score of block at coordinate $(bx,by)$
 */
void AbstractBlockAligner::pruningUpdate(int bx, int by, int score) {
	if (blockPruner != NULL) {
		blockPruner->pruningUpdate(bx, by, score);
	}
}

/**
 * Defines the preferred block/grid sizes. Used as a hint.
 *
 * @param preferredBlockSize the preferred maximum size of a block.
 * @param preferredGridSize the preferred minimum grid size.
 */
void AbstractBlockAligner::setPreferredSizes(int preferredBlockSize,	int preferredGridSize) {
	if (preferredBlockSize > 0) {
		this->preferredBlockSize = preferredBlockSize;
	}
	if (preferredGridSize > 0) {
		this->preferredGridSize = preferredGridSize;
	}
}




