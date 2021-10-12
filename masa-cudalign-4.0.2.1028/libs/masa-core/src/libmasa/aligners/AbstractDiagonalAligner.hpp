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

#ifndef ABSTRACTDIAGONALALIGNER_HPP_
#define ABSTRACTDIAGONALALIGNER_HPP_

#include "../libmasa.hpp"

#include "../pruning/BlockPruningDiagonal.hpp"

/**
 * @brief Abstract class that processes diagonal of blocks.
 *
 * This class implements some common behavior to simplify the implementation
 * of diagonal aligners. Use the AbstractDiagonalAligner if you want to compute
 * all blocks inside the diagonal at once inside the architecture hardware/software.
 * Differently from the AbstractBlockAligner,
 * the AbstractDiagonalAligner does not use an AbstractBlockProcessor object,
 * what means that the Aligner must compute the blocks by themselves.
 *
 * In order to extend an AbstractDiagonalAligner, the class must implement
 * two group of methods.
 *
 * <ul>
 *  <li>Memory related methods: get/set methods that obtain the
 *  	results from the specific architecture, such as block scores and
 *  	special rows/columns.
 *  <li>Iteration methods: responsible to execute the diagonal iterations.
 *      The diagonals are numbered from 0. Generically speaking, diagonal
 *      with number $d$ is expected to compute all blocks $(bx,by)$ such that
 *      $bx+by=d$ is true.
 * </ul>
 *
 * The memory related methods are called considering the order of the executed
 * iteration, so we guarantee that a special row/column will only be issued
 * when it is already computed.
 */
class AbstractDiagonalAligner : public AbstractAligner {
public:
	/* Constructors */

	AbstractDiagonalAligner();
	virtual ~AbstractDiagonalAligner();

	/* Implementation of virtual methods from IAligner */

	virtual void alignPartition(Partition partition);

	virtual void clearStatistics();
	virtual void printInitialStatistics(FILE* file);
	virtual void printStageStatistics(FILE* file);
	virtual void printFinalStatistics(FILE* file);
	virtual void printStatistics(FILE* file);
	virtual long long getProcessedCells();
	virtual const char* getProgressString() const;

protected:

	/* Virtual methods that must be implemented by subclasses */

	/**
	 * Returns the number of horizontal blocks that the grid must have
	 * for a given width partition.
	 *
	 * @param width the width of the partition to be aligned.
	 * @return the recommended grid width for that partition.
	 */
	virtual int getGridWidth(int width) = 0;

	/**
	 * Returns the default height of each block. All the blocks will have
	 * the same height except the bottom most rows, that may be truncated.
	 *
	 * @return the default height of each block.
	 */
	virtual int getBlockHeight() = 0;

	/**
	 * Returns the bottom-most cells of a block computed from the previous
	 * computed diagonal. The block cells range is in the interval $[j,j+len)$.
	 * Since we are processing in diagonals, only one block can reside in this
	 * range. The cells will be saved in a special row by MASA-Core.
	 *
	 * @param j the start column of the cells to be returned.
	 * @param len the number of cells to be returned.
	 * @return the vector containing the special cells.
	 */
	virtual const cell_t* getSpecialRow(int j, int len) = 0;

	/**
	 * Returns the cells in the interval $[j,j+len)$ of the bottom-most row.
	 *
	 * @param l the start column of the cells to be returned.
	 * @param len the number of cells to be returned.
	 * @return the vector containing the special cells.
	 */
	virtual const cell_t* getLastRow(int j, int len) = 0;

	/**
	 * Returns the cells in the interval $[i,i+len)$ of the bottom-most row.
	 *
	 * @param i the start column of the cells to be returned.
	 * @param len the number of cells to be returned.
	 * @return the vector containing the special cells.
	 */
	virtual const cell_t* getLastColumn(int i, int len) = 0;

	/**
	 * Returns the best scores of the blocks from the last computed diagonal.
	 * @return the best scores of each blocks (ordered from left to right).
	 */
	virtual const score_t* getBlockScores() = 0;

	/**
	 * Initializes the cells in the range $[j,j+len)$ of the first row.
	 * @param cells the vector containing the cells.
	 * @param j the start column of the cells to be initialized.
	 * @param len the number of cells to be initialized.
	 */
	virtual void setFirstRow(const cell_t* cells, int j, int len) = 0;

	/**
	 * Initializes the cells in the range $[i,i+len)$ of the first column.
	 * @param cells the vector containing the cells.
	 * @param i the start column of the cells to be initialized.
	 * @param len the number of cells to be initialized.
	 */
	virtual void setFirstColumn(const cell_t* cells, int i, int len) = 0;

	/**
	 * Clear the rows of the pruned blocks, in range $[b0,b1)$. The rows
	 * must be cleared with very small numbers (-INF) in order to avoid
	 * garbage data in the special rows or in future non-pruned blocks
	 * that may use the same area in memory.
	 *
	 * @param b0 first block to be cleared (inclusive).
	 * @param b1 last block to be cleared (exclusive)
	 */
	virtual void clearPrunedBlocks(int b0, int b1) = 0;

	/**
	 * Executes initialization procedures before the first diagonal be processed.
	 */
	virtual void initializeDiagonals() = 0;

	/**
	 * Executes one diagonal with the blocks from $[windowLeft, windowRight]$.
	 * The diagonals are numbered from 0, but the first diagonal (0) is not
	 * expected to compute any block. Diagonal 1 is expected to compute
	 * block $(0,0)$, diagonal 2 is expected to compute blocks $(1,0)$ and
	 * $(0,1)$, and so on. Generically speaking, diagonal $d$ is expected
	 * to compute all blocks $(bx,by)$ that $bx+by=d$.
	 *
	 * @param diagonal the diagonal number to be processed.
	 * @param windowLeft the first block to be processed (inclusive).
	 * @param windowRight the last block to be processed (inclusive).
	 */
	virtual void processDiagonal(int diagonal, int windowLeft, int windowRight) = 0;

	/**
	 * Executes finalization procedures after the last diagonal is processed.
	 */
	virtual void finalizeDiagonals() = 0;


	/* Other protected methods*/

	Partition getPartition() const;
private:
	/**
	 * Vector used to store the cells of the first column.
	 * @see AbstractDiagonalAligner::loadFirstColumn() method.
	 */
	cell_t*  h_loadColumn;

	/** number of columns of blocks */
	int gridWidth;
	/** number of rows of blocks */
	int gridHeight;

	/** Number of external diagonals to be processed in the current partition.*/
	int externalDiagonalCount;
	/** The id of the current external diagonal being processed. */
	int currentExternalDiagonal;

	/** The partition currently being processed */
	Partition partition;
	/** Block Pruner object */
	BlockPruningDiagonal* pruner;

	/** First block of non-pruned window */
	int windowStart;
	/** Last block of non-pruned window */
	int windowEnd;

	/*
	 * The following attributes are used for statistics purpose only.
	 */
	/** Total number of blocks containing in the grid */
	int statTotalBlocks;
	/** Number of pruned blocks in the left side of the grid */
	int statPrunedBlocksLeft;
	/** Number of pruned blocks in the left side of the grid */
	int statPrunedBlocksRight;
	/** Total number of cells in the grid */
	long long statTotalCells;
	/** Maintains the minimum gridWidth used. */
	int statMinGridWidth;
	/** Maintains the maximum gridWidth used. */
	int statMaxGridWidth;

	/* Iteration related methods */

	Grid* configureGrid(Partition partition);
	void prepareIterations();
	void processNextIteration();
	bool hasMoreIterations();
	void finalizeIterations();

	/* ``flushXXX'' methods dispatch data from the Aligner to MASA-Core. */

	void flushSpecialRows();
	void flushLastRow();
	void flushLastColumn();
	void flushLastCell();
	void flushBlockScores();

	/* ``loadXXX'' methods receives data from MASA-Core and send them to the Aligner. */

	void loadFirstRow();
	void loadFirstColumn();

	/* Other methods */

	bool isSpecialRow(int by);
	void pruneBlocks();
};

#endif /* ABSTRACTDIAGONALALIGNER_HPP_ */
