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

#ifndef ALIGNERMANAGER_HPP_
#define ALIGNERMANAGER_HPP_

#include "../libmasa/libmasa.hpp"

//#include "buffer/Buffer.hpp"
//#include "BlocksFile.hpp"
#include "biology/Sequence.hpp"
#include "io/CellsReader.hpp"
#include "io/CellsWriter.hpp"
#include "sra/SpecialRowsPartition.hpp"
#include "BlocksFile.hpp"
#include "BestScoreList.hpp"
#include "Job.hpp"

typedef void (*callback_f)(int i, int j, int len, cell_t* data);
typedef void (*callback_score_f)(score_t score, int bx, int by);


class AlignerManager : public IManager {
public:
	AlignerManager(IAligner* aligner);
	virtual ~AlignerManager();

	void alignPartition(Partition partition, int startType);

	/**
	 * Defines the partition that must be processed.
	 * @param i0	start row
	 * @param j0	start column
	 * @param i1	end row (exclusive)
	 * @param j1	end column (exclusive)
	 * @param start_type The start type of the partition. See AbstractAligner::startType.
	 */
	//void setPartition(Partition partition, int start_type);

	/**
	 * Defines the sequences and the range of the sequences
	 * that will be aligned. This method also defines the range of
	 * the sequences that will be used in the alignPartitions. Note
	 * that the [i0,i1) and [j0,j1) are not the partition itself, but
	 * all the aligned partitions will reside inside this range. If the
	 * parameters $i0$ and $j0$ are positive, then the sequence data
	 * passed to the aligner is shifted, starting in positions $i0$ and $j0$.
	 * This allows the reduction of memory consumption, trimming the
	 * prefix of the sequences that would never be used (i.e.
	 * the memory may be allocated solely for the given sequence ranges.).
	 *
	 * @param seq0	vertical sequence object.
	 * @param seq1	horizontal sequence object.
	 * @param i0	lowest position in seq0 that may be aligned.
	 * @param j0	lowest position in seq1 that may be aligned.
	 * @param i1	highest position in seq0 that may be aligned.
	 * @param j1	highest position in seq1 that may be aligned.
	 * @param stats the statistics log file
	 */
	void setSequences(Sequence* seq0, Sequence* seq1, int i0, int j0, int i1, int j1, FILE* stats=NULL);

	/**
	 * Clear all sequence strucutures.
	 */
	void unsetSequences();

	/**
	 * Defines that the first column must be initialized with a pre-defined
	 * column.
	 *
	 * @param firstColumnGapped If true, than the cells must be initialized
	 * considering gaps, otherwise the cells must be initialized with zeros.
	 * See AbstractAligner::getFirstColumnInitType for the initialization
	 * functions.
	 */
	//void setFirstColumnSource(int firstColumnInitType);

	/**
	 * Defines the recurrence type of the execution.
	 * @param recurrenceType can be SMITH_WATERMAN or NEEDLEMAN_WUNSCH.
	 */
	void setRecurrenceType(int recurrenceType);

	/**
	 * Defines if the aligner may do the block pruning optimization. Since there
	 * are situations that the block pruning is not permitted, so the aligner
	 * must respect when blockPruning is false.
	 *
	 * @param blockPruning true if the aligner may prune blocks.
	 */
	void setBlockPruning(bool blockPruning);

	/**
	 * Defines the grid dimension of the grid, if
	 * capabilities_t::dispatch_block_scores is SUPPORTED.
	 *
	 * @param width the width of the grid.
	 * @param height the height of the grid.
	 */
	//void setGridDimensions(const int width, const int height);

	/**
	 * Defines the partition where the special rows will be stored.
	 * @param specialRowsPartition the special row partition.
	 */
	void setSpecialRowsPartition(SpecialRowsPartition* specialRowsPartition);

	/**
	 * Defines the list to store the best scores.
	 * @param bestScoreList the list to store the best scores.
	 * @param bestScoreLocation where to check for best score.
	 */
	void setBestScoreList(BestScoreList* bestScoreList, const int bestScoreLocation = AT_NOWHERE);

	/**
	 * Whenever the aligner finds the goal score, the processing must stop.
	 */
	void setGoalScore(int goalScore, const int goalScoreLocation = AT_NOWHERE);

	/**
	 * Defines the previous special rows partition containing the rows to
	 * be matched against the last column/rows.
	 */
	void setLastColumnReader(SeekableCellsReader* lastColumnReader);

	/**
	 * Defines the previous special rows partition containing the rows to
	 * be matched against the last column/rows.
	 */
	void setLastRowReader(SeekableCellsReader* lastRowReader);

	/**
	 * Do not find best score
	 */
	void unsetGoalScore();

	/**
	 * Defines the file to store the best score of each block.
	 * @param blocksFile the file to store the scores.
	 */
	void setBlocksFile(BlocksFile* blocksFile);

	/**
	 * Defines the minimum distance (in rows) between two special rows.
	 *
	 * @param specialRowInterval the minimum interval between special rows.
	 */
	void setSpecialRowInterval(const int specialRowInterval);

	/**
	 * Defines the variable penalty functions to be aligned.
	 *
	 * @param match		Match score
	 * @param mismatch	Mismatch score
	 * @param gapOpen	Gap opening penalty
	 * @param gapExtension	Gap extension penalty
	 */
	void setPenalties(const int match, const int mismatch, const int gapOpen, const int gapExtension);

	/**
	 * Sets the super-partition being aligned to be returned by the
	 * IManager::getSuperPartition() method.
	 *
	 * @param superPartition the super partition to be set.
	 * @see IManager::getSuperPartition()
	 */
	void setSuperPartition(Partition superPartition);

	/**
	 * Resets the super-partition. A reseted super-partition means that
	 * it will always be equal to the current aligned partition.
	 */
	void unsetSuperPartition();

	/* ********************* *
	 *  Callback functions   *
	 * ********************* */

	/**
	 * Defines the callback function that will be called whenever the
	 * dispatchRow method is called for the last row.
	 *
	 * @param processLastRowFunction the callback function.
	 */
	//void setProcessLastRowFunction(callback_f processLastRowFunction);

	/**
	 * Defines the callback function that will be called whenever the
	 * dispatchColumn method is called for the last column.
	 *
	 * @param processLastColumnFunction the callback function.
	 */
	//void setProcessLastColumnFunction(callback_f processLastColumnFunction);

	/**
	 * Defines the callback function that will be called whenever the
	 * dispatchScore method is called.
	 *
	 * @param processBlockFunction the callback function.
	 */
	//void setProcessScoreFunction(callback_score_f processBlockFunction);

	/**
	 * Defines the callback function that will be called whenever the
	 * dispatchScore method is called for the last cell (right-bottom most cell).
	 *
	 * @param processLastCellFunction the callback function.
	 */
	//void setProcessLastCellFunction(callback_score_f processLastCellFunction);

	/**
	 * Return the next crosspoint found by the matching procedure or by the
	 * goal score. If it was not found, the returned crosspoint score is -INF.
	 */
	const crosspoint_t getNextCrosspoint() const;

	/**
	 * Return true if the next crosspoint was found.
	 */
	bool isFoundCrosspoint() const;

	/* Implementing IManager methods */

	/* Get methods */
	int getRecurrenceType() const;
	int getSpecialRowInterval() const;
	int getSpecialColumnInterval() const;
	int getFirstColumnInitType();
	int getFirstRowInitType();
	Partition getSuperPartition();

	/* Receive Methods */
	void receiveFirstRow(cell_t* buffer, int len);
	void receiveFirstColumn(cell_t* buffer, int len);

	/* Dispatch Methods */
	void dispatchColumn(int j, const cell_t* buffer, int len);
	void dispatchRow(int i, const cell_t* buffer, int len);
	void dispatchScore(score_t score, int bx=-1, int by=-1);

	/* Must Methods */
	bool mustContinue();
	bool mustDispatchLastCell();
	bool mustDispatchLastRow();
	bool mustDispatchLastColumn();
	bool mustDispatchSpecialRows();
	bool mustDispatchSpecialColumns();
	bool mustDispatchScores();
	bool mustPruneBlocks();
	score_t getBestScoreLastColumn() const;
	score_t getBestScoreLastRow() const;

private:
	bool active;

	/** The aligner object that executes the SW computation */
	IAligner* aligner;

	/** The partition that is being aligned */
	Partition partition;

	/**
	 * The reader that provides the last column saved in memory/disk.
	 */
	SeekableCellsReader* lastColumnReader;

	/**
	 * The reader that provides the last rows saved in memory/disk.
	 */
	SeekableCellsReader* lastRowReader;

	/**
	 * Vector that store the temporary cells for the matching procedures.
	 */
	cell_t* baseColumn;

	/**
	 * Vector that store the temporary cells for the matching procedures.
	 */
	cell_t* baseRow;

	/** Math/mismatch/gaps parameters */
	const score_params_t* score_params;

	/**
	 * The start type of the partition. Possible values are: TYPE_MATCH,
	 * TYPE_GAP_1 or TYPE_GAP_2. When set to the TYPE_GAP_1 or TYPE_GAP_2,
	 * the initialization of the first row/column must be done without the
	 * gap opening penalty.
	 */
	int startType;

	/** Match Score */
	int match;

	/** Mismatch Score */
	int mismatch;

	/** Gap opening penalty */
	int gapOpen;

	/** Gap extension penalty */
	int gapExtension;

	/** First column blocking buffer */
	SeekableCellsReader* firstColumnReader;

	/** First row blocking buffer */
	SeekableCellsReader* firstRowReader;

	/** Last column blocking buffer */
	CellsWriter* lastColumnWriter;

	/** Last row blocking buffer */
	CellsWriter* lastRowWriter;

	/** Partition where the Special Rows are stored */
	SpecialRowsPartition* specialRowsPartition;

	/** List with the best scores */
	BestScoreList* bestScoreList;

	/** The aligner must stop whenever it finds the goal score. */
	int goalScore;

	/** Where to check the goal score */
	int goalScoreLocation;

	/** Indicates if the next crosspoint has already been found. */
	bool foundCrosspoint;

	/** The crosspoint found by the matching procedure or the goal score */
	crosspoint_t nextCrosspoint;

	/* Stores the score of each block */
	BlocksFile* blocksFile;

	/** Where to check best score */
	int bestScoreLocation;

	/** Column tracking position for the dispatching procedure */
	int lastColumnPos;

	/** Row tracking position for the dispatching procedure */
	int lastRowPos;

	/** Callback for the last rows */
	//callback_f processLastRowFunction;

	/** Callback for the last cells */
	//callback_score_f processLastCellFunction;

	/** Callback for the scores */
	//callback_score_f processScoreFunction;

	/** Callback for the last score */
	//callback_f processLastColumnFunction;

	/** true if block must be pruned */
	int blockPruning;

	/** required recurrence type (SMITH_WATERMAN or NEEDLEMAN_WUNSCH) */
	int recurrenceType;

	/** minimum distance between two special rows */
	int specialRowInterval;

	/** File where the first row is stored */
	FILE* firstRowFile;

	/** First row tracking position for the receive procedure */
	//int firstRowPos;

	/** First column tracking position for the receive procedure */
	//int firstColumnPos;

	int firstColumnInitType;

	/** true if first row is gapped */
	//bool firstRowGapped;

	int firstRowInitType;

	/** true if first columns is gapped */
	//bool firstColumnGapped;

	score_t bestScoreLastColumn;
	score_t bestScoreLastRow;

	/*
	 * If the setSequence method defines that the sequence may only be aligned
	 * after some position, then the aligner receive a trimmed sequence and
	 * its partition coordinates are relative to the start of the trimmed
	 * sequences. This reduces the amount of memory used by the aligner.
	 * The seq0_offset and seq1_offset indicates how many nucleotides
	 * were trimmed in each sequence.
	 */
	/** defines how many nucleotides were trimmed from the sequence 0 */
	int seq0_offset;
	/** defines how many nucleotides were trimmed from the sequence 1 */
	int seq1_offset;

	/**
	 * Partition that holds all sub-partitions. If there are no
	 * sub-partition, this is the single partition being aligned.
	 */
	Partition superPartition;

	/**
	 * Stops the execution of the aligner. This makes the
	 * mustContinue() method to return false.
	 */
	void stopAligner();


	int findBestCell(const cell_t* buffer, int len);
	match_result_t findGoalCell(const cell_t* buffer, cell_t* base, int len, CellsReader* cellsReader);
	match_result_t findFullGap(int len, bool openGap, SeekableCellsReader* cellsReader);

	/**
	 * Defines that the first column must be initialized with a customized
	 * column. The column is loaded from a Blocking Buffer, so consider that
	 * the buffer will block the process if the requested data is not
	 * fully ready. So, the buffer must be read in chunks, in order to not
	 * block the entire execution.
	 *
	 * @param firstColumnBuffer the blocking buffer that will contain the
	 * first column data.
	 */
	void setFirstColumnSource(SeekableCellsReader* firstColumnReader);

	/**
	 * Defines that the first row must be initialized with a pre-defined
	 * row.
	 *
	 * @param firstRowGapped If true, than the cells must be initialized
	 * considering gaps, otherwise the cells must be initialized with zeros.
	 * See AbstractAligner::getFirstRowInitType for the initialization
	 * functions.
	 */
	//void setFirstRowSource(int firstRowInitType);

	/**
	 * Defines that the first row must be initialized with a customized
	 * row. The column is loaded from a FILE.
	 *
	 * @param firstRow the file that contains the first column data.
	 */
	void setFirstRowSource(SeekableCellsReader* firstColumnReader);

	/**
	 * Defines the destination of the last column. The column is stored
	 * in a Blocking Buffer, so consider that the buffer will block the
	 * process if the buffer is full.

	 * @param lastColumnBuffer the buffer that will receive the data of
	 * 	the last column.
	 */
	void setLastColumnDestination(CellsWriter* lastColumnWriter);



	void setLastRowDestination(CellsWriter* lastRowWriter);

};

#endif /* ALIGNERMANAGER_HPP_ */
