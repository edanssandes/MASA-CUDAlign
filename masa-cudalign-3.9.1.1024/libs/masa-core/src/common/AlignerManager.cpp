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

#include "AlignerManager.hpp"
#include <stdlib.h>
#include "io/InitialCellsReader.hpp"

#define DEBUG (0)

/** Maximum number of cells computed per iteration in the matching procedure */
#define BUS_BASE_SIZE	(1*1024) // 1K cells

AlignerManager::AlignerManager(IAligner* aligner) {
	this->aligner = aligner;
	this->score_params = aligner->getScoreParameters();

	/* Clear the values of all structures */

	this->lastColumnWriter = NULL;
	this->lastRowWriter = NULL;
	this->firstColumnReader = NULL;
	this->firstRowFile = NULL;

	this->specialRowsPartition = NULL;
	this->bestScoreList = NULL;
	this->bestScoreLocation = AT_NOWHERE;
	this->goalScore = -INF;
	this->foundCrosspoint = false;
	this->nextCrosspoint.score = -INF;
	this->nextCrosspoint.i = -1;
	this->nextCrosspoint.j = -1;

	this->blocksFile = NULL;

	unsetSuperPartition();

	/*this->processLastCellFunction = NULL;
	this->processLastColumnFunction = NULL;
	this->processLastRowFunction = NULL;
	this->processScoreFunction = NULL;*/

	this->recurrenceType = SMITH_WATERMAN;
	this->blockPruning = false;
	//this->firstColumnGapped = false;
	//this->firstRowGapped = false;

	this->baseColumn = (cell_t*)malloc(BUS_BASE_SIZE*sizeof(cell_t));
	this->baseRow = (cell_t*)malloc(BUS_BASE_SIZE*sizeof(cell_t));

	this->lastColumnReader = NULL;
	this->lastRowReader = NULL;

	aligner->setManager(this);
}

AlignerManager::~AlignerManager() {
	/* Deallocate and close every previously allocated resource */

	if (firstRowFile != NULL) {
		fclose(firstRowFile);
		firstRowFile = NULL;
	}

	if (this->baseColumn != NULL) {
		free(this->baseColumn);
		this->baseColumn = NULL;
	}
}

/*
 * @see definition on header file
 */
void AlignerManager::alignPartition(Partition partition, int start_type) {
	this->partition = partition;
	this->startType = start_type;
	this->foundCrosspoint = false;

	if (partition.getWidth() == 0 || partition.getHeight() == 0) {
		fprintf(stderr, "Zero-area partition. Skipping.\n");
		return;
	}

	lastColumnPos = 0; // TODO uniformizar sem +1
	lastRowPos = 0;

	bestScoreLastColumn.score = -INF;
	bestScoreLastColumn.i = -1;
	bestScoreLastColumn.j = -1;
	bestScoreLastRow.score = -INF;
	bestScoreLastRow.i = -1;
	bestScoreLastRow.j = -1;

	//this->firstColumnPos = 1;
	//this->firstRowPos = 0;

	this->active = true;

	if (specialRowsPartition != NULL) {
		setFirstRowSource(specialRowsPartition->getFirstRowReader());
		setFirstColumnSource(specialRowsPartition->getFirstColumnReader());
		setLastColumnDestination(specialRowsPartition->getLastColumnWriter());
		setLastRowDestination(specialRowsPartition->getLastRowWriter());
	}

	// TODO desalocar o firstColumn reader quando alocado internamente
	// verificar se existe algum cenário onde firstColumnReader == NULL
//	if (firstColumnReader != NULL) {
//		cell_t tmp;
//		firstColumnReader->read(&tmp, 1);
//	}

//	if (mustDispatchLastColumn()) {
//		cell_t first_cell;
//		first_cell.h = partition.getWidth() *-score_params->gap_ext - (start_type==TYPE_GAP_1 ? 0 : score_params->gap_open);
//		first_cell.f = first_cell.h;
//	    dispatchColumn(partition.getJ1(), &first_cell, 1);
//	}

	if (goalScoreLocation == AT_SEQUENCE_2 || goalScoreLocation == AT_SEQUENCE_1_OR_2) {
		match_result_t result = findFullGap(partition.getWidth(), start_type!=TYPE_GAP_1, lastColumnReader);
		if (result.found) {
			nextCrosspoint.j = partition.getJ1();
			nextCrosspoint.i = partition.getI0();
			nextCrosspoint.score = result.score;
			nextCrosspoint.type = TYPE_GAP_1;
			if (DEBUG) printf ( "-- Crosspoint found on last column: (%d,%d) - %d [%d] FULL GAP!\n",
					nextCrosspoint.i, nextCrosspoint.j, nextCrosspoint.score, nextCrosspoint.type);
			this->active = false;
		}
	}

	if (goalScoreLocation == AT_SEQUENCE_1 || goalScoreLocation == AT_SEQUENCE_1_OR_2) {
		match_result_t result = findFullGap(partition.getHeight(), start_type!=TYPE_GAP_2, lastRowReader);
		if (result.found) {
			nextCrosspoint.j = partition.getJ0();
			nextCrosspoint.i = partition.getI1();
			nextCrosspoint.score = result.score;
			nextCrosspoint.type = TYPE_GAP_2;
			if (DEBUG) printf ( "-- Crosspoint found on last row: (%d,%d) - %d [%d] FULL GAP!\n",
					nextCrosspoint.i, nextCrosspoint.j, nextCrosspoint.score, nextCrosspoint.type);
			this->active = false;
		}
	}


	if (this->active) {
		Partition partition_adj = Partition(partition, -seq0_offset, -seq1_offset);
		aligner->alignPartition(partition_adj);
	}
}

/*
 * @see definition on header file
 */
void AlignerManager::setSequences(Sequence* seq0, Sequence* seq1, int i0, int j0, int i1, int j1, FILE* stats) {
	if (DEBUG) printf("AlignerManager::setSequences(%p, %p, %d, %d, %d, %d, %p)\n", seq0, seq1, i0, j0, i1, j1, stats);
	seq0_offset = i0;
	seq1_offset = j0;
	aligner->setSequences(seq0->getData()+seq0_offset, seq1->getData()+seq1_offset, i1-i0, j1-j0);
	if (stats != NULL) {
		aligner->printStageStatistics(stats);
	}
}

/*
 * @see definition on header file
 */
void AlignerManager::unsetSequences() {
	aligner->unsetSequences();
}



/**
 *
 */
void AlignerManager::setBlockPruning(bool blockPruning) {
	this->blockPruning = blockPruning;
}

/*
 * @see definition on header file
 */
//void AlignerManager::setFirstColumnSource(int firstColumnInitType) {
//	this->firstColumnInitType = firstColumnInitType;
//	if (firstColumnInitType == INIT_WITH_GAPS_OPENED) {
//		this->firstColumnReader = new InitialCellsReader(0, score_params->gap_ext);
//	} else if (firstColumnInitType == INIT_WITH_GAPS) {
//		this->firstColumnReader = new InitialCellsReader(score_params->gap_open, score_params->gap_ext);
//	} else if (firstColumnInitType == INIT_WITH_ZEROES) {
//		this->firstColumnReader = new InitialCellsReader();
//	}
//}

/*
 * @see definition on header file
 */
void AlignerManager::setFirstColumnSource(SeekableCellsReader* firstColumnReader) {
	firstColumnInitType = firstColumnReader->getType();
	this->firstColumnReader = firstColumnReader;

//	if (this->firstColumnReader->getType() != INIT_WITH_CUSTOM_DATA) {
//		this->firstColumnReader->seek(0);
//	}

//	if (specialRowsPartition != NULL) {
//		specialRowsPartition->setFirstColumnReader(firstColumnReader);
//	}

}

/*
 * @see definition on header file
 */
//void AlignerManager::setFirstRowSource(int firstRowInitType) {
//	this->firstRowInitType = firstRowInitType;
//	if (firstRowInitType == INIT_WITH_GAPS_OPENED) {
//		this->firstRowReader = new InitialCellsReader(0, score_params->gap_ext);
//	} else if (firstRowInitType == INIT_WITH_GAPS) {
//		this->firstRowReader = new InitialCellsReader(score_params->gap_open, score_params->gap_ext);
//	} else if (firstRowInitType == INIT_WITH_ZEROES) {
//		this->firstRowReader = new InitialCellsReader();
//	}
//}

/*
 * @see definition on header file
 */
void AlignerManager::setFirstRowSource(SeekableCellsReader* firstRowReader) {
	this->firstRowInitType = firstRowReader->getType();
	this->firstRowReader = firstRowReader;

//	if (this->firstRowReader->getType() != INIT_WITH_CUSTOM_DATA) {
//		this->firstRowReader->seek(0);
//	}

//	if (specialRowsPartition != NULL) {
//		specialRowsPartition->setFirstRowReader(firstRowReader);
//	}
}

/*
 * @see definition on header file
 */
void AlignerManager::setLastColumnDestination(CellsWriter* lastColumnWriter) {
	this->lastColumnWriter = lastColumnWriter;
}


Partition AlignerManager::getSuperPartition() {
	if (superPartition.getJ0() == -1) {
		return Partition(partition, -seq0_offset, -seq1_offset);
	} else {
		return Partition(superPartition, -seq0_offset, -seq1_offset);
	}
}

void AlignerManager::setSuperPartition(Partition superPartition) {
	this->superPartition = superPartition;
}

void AlignerManager::unsetSuperPartition() {
	this->superPartition = Partition(-1,-1,-1,-1);
}

void AlignerManager::setLastRowDestination(CellsWriter* lastRowWriter) {
	this->lastRowWriter = lastRowWriter;
}

void AlignerManager::setLastColumnReader(SeekableCellsReader* lastColumnReader) {
	this->lastColumnReader = lastColumnReader;
}

void AlignerManager::setLastRowReader(SeekableCellsReader* lastRowReader) {
	this->lastRowReader = lastRowReader;
}

/*
 * @see definition on header file
 */
void AlignerManager::setRecurrenceType(int recurrenceType) {
	this->recurrenceType = recurrenceType;
}

/*
 * @see definition on header file
 */
void AlignerManager::setSpecialRowsPartition(SpecialRowsPartition* specialRowsPartition) {
	this->specialRowsPartition = specialRowsPartition;
}

/*
 * @see definition on header file
 */
void AlignerManager::setSpecialRowInterval(const int specialRowInterval) {
	this->specialRowInterval = specialRowInterval;
}


void AlignerManager::receiveFirstColumn(cell_t* buffer, int len) {
	if (DEBUG) printf ( "AlignerManager::receiveFirstColumn(..,%d)\n", len);
	firstColumnReader->read(buffer, len);
}

/*
 * @see definition on header file
 */
void AlignerManager::receiveFirstRow(cell_t* buffer, int len) {
	if (DEBUG) printf ( "AlignerManager::receiveFirstRow(..,%d)\n", len);
	firstRowReader->read(buffer, len);
}

/*
 * @see definition on header file
 */
void AlignerManager::dispatchColumn(int j, const cell_t* buffer, int len) {
	j += seq1_offset;
	if (DEBUG) printf ( "AlignerManager::dispatchColumn (%d,..,%d) %d %s\n", j, len, partition.getJ1(), j==partition.getJ1()?"LAST COL":"");

	//printf("%d != %d \n", j0+j, j1);
	if (j == partition.getJ1()) {
		int best_id = findBestCell(buffer, len);
		score_t score_adj;
		score_adj.score = buffer[best_id].h;
		score_adj.j = partition.getJ1();
		score_adj.i = partition.getI0()+lastColumnPos+best_id;
		if (bestScoreLastColumn.score <= score_adj.score) {
			bestScoreLastColumn = score_adj;
		}

		if (lastColumnWriter != NULL) {
			lastColumnWriter->write(buffer, len);
		}
		if (bestScoreLocation == AT_SEQUENCE_2 || bestScoreLocation == AT_SEQUENCE_1_OR_2) {
			bestScoreList->add(score_adj.i, score_adj.j, score_adj.score);
		}
		if (goalScoreLocation == AT_ANYWHERE || goalScoreLocation == AT_SEQUENCE_2 || goalScoreLocation == AT_SEQUENCE_1_OR_2) {
			match_result_t result = findGoalCell(buffer, baseColumn, len, lastColumnReader);
			if (result.found) {
				nextCrosspoint.j = partition.getJ1();
				nextCrosspoint.i = partition.getI0()+lastColumnPos+result.k;
				nextCrosspoint.score = result.score;
				nextCrosspoint.type = result.type==MATCH_ALIGNED ? TYPE_MATCH : TYPE_GAP_1;
				if (DEBUG) printf ( "-- Crosspoint found on last column: (%d,%d) - %d [%d]!\n",
						nextCrosspoint.i, nextCrosspoint.j, nextCrosspoint.score, nextCrosspoint.type);
				stopAligner();
			}
		}
		lastColumnPos += len;
	}
}

/*
 * @see definition on header file
 */
void AlignerManager::dispatchRow(int i, const cell_t* buffer, int len) {
	i += seq0_offset;
	if (DEBUG) printf ( "AlignerManager::dispatchRow (%d,..,%d) %d %s\n", i, len, partition.getI1(), i==partition.getI1()?"LAST ROW":"");
	if (mustDispatchSpecialRows()) {
		specialRowsPartition->write(i, buffer, len);
	}
	if (i == partition.getI1()) {
		if (lastRowWriter != NULL) {
			lastRowWriter->write(buffer, len);
		}
		if (bestScoreLocation == AT_SEQUENCE_1 || bestScoreLocation == AT_SEQUENCE_1_OR_2) {
			int best_id = findBestCell(buffer, len);
			score_t score_adj;
			score_adj.score = buffer[best_id].h;
			score_adj.j = partition.getJ0()+lastRowPos+best_id;
			score_adj.i = partition.getI1();
			bestScoreList->add(score_adj.i, score_adj.j, score_adj.score);
		}
		if (goalScoreLocation == AT_ANYWHERE || goalScoreLocation == AT_SEQUENCE_1 || goalScoreLocation == AT_SEQUENCE_1_OR_2) {
			match_result_t result = findGoalCell(buffer, baseRow, len, lastRowReader);
			if (result.found) {
				nextCrosspoint.i = partition.getI1();
				nextCrosspoint.j = partition.getJ0()+lastRowPos+result.k;
				nextCrosspoint.score = result.score;
				nextCrosspoint.type = result.type==MATCH_ALIGNED ? TYPE_MATCH : TYPE_GAP_2;
				if (DEBUG) printf ( "-- Crosspoint found on last row: (%d,%d) - %d [%d]!\n",
						nextCrosspoint.i, nextCrosspoint.j, nextCrosspoint.score, nextCrosspoint.type);
				stopAligner();
			}
		}
		lastRowPos += len;
	}
}

/*
 * @see definition on header file
 */
void AlignerManager::dispatchScore(score_t score, int bx, int by) {
	score_t score_adj;
	score_adj.i = score.i + seq0_offset + 1;
	score_adj.j = score.j + seq1_offset + 1;
	score_adj.score = score.score;
	if (DEBUG) printf ( "AlignerManager::dispatchScore (%d,%d,%d) - block [%d,%d]\n", score_adj.i, score_adj.j, score_adj.score, bx, by);

	if (blocksFile != NULL && bx != -1 && by != -1) {
		if (!blocksFile->isInitialized()) {
			blocksFile->initialize(aligner->getGrid());
		}
		blocksFile->setScore(bx, by, score_adj.score);
	}
	if (score_adj.score > -INF) {
		if (bestScoreLocation == AT_ANYWHERE) {
			bestScoreList->add(score_adj.i, score_adj.j, score_adj.score);
		} else if (bestScoreLocation == AT_SEQUENCE_1_AND_2) {
			if (score_adj.i == partition.getI1() && score_adj.j == partition.getJ1()) {
				bestScoreList->add(score_adj.i, score_adj.j, score_adj.score);
			}
		}
		if (goalScoreLocation == AT_ANYWHERE) {
			if (score_adj.score == goalScore) {
				nextCrosspoint.i = score_adj.i;
				nextCrosspoint.j = score_adj.j;
				nextCrosspoint.score = 0;
				nextCrosspoint.type = 0;
				foundCrosspoint = true;
				stopAligner();
				if (DEBUG) printf ( ":GOAL END (%d,%d) - %d\n", nextCrosspoint.i, nextCrosspoint.j, nextCrosspoint.score);
			}
		}
	}
	/*if (processScoreFunction != NULL) {
		processScoreFunction(score_adj, bx, by);
	}
	if (processLastCellFunction != NULL && score_adj.i == partition.getI1()-1 && score_adj.j == partition.getJ1()-1) {
		processLastCellFunction(score_adj, bx, by);
	}*/
}

/*
 * @see definition on header file
 */
bool AlignerManager::mustDispatchLastCell() {
	return /*processLastCellFunction != NULL
			||*/ (bestScoreLocation == AT_SEQUENCE_1_AND_2);
}

/*
 * @see definition on header file
 */
bool AlignerManager::mustDispatchLastRow() {
	return /*processLastRowFunction != NULL
			||*/
			(lastRowWriter != NULL)
			|| (bestScoreLocation == AT_SEQUENCE_1 || bestScoreLocation == AT_SEQUENCE_1_OR_2)
			|| ((goalScoreLocation == AT_ANYWHERE || goalScoreLocation == AT_SEQUENCE_1 || goalScoreLocation == AT_SEQUENCE_1_OR_2) && lastRowReader != NULL)
			;
}

/*
 * @see definition on header file
 */
bool AlignerManager::mustDispatchLastColumn() {
	return /*(processLastColumnFunction != NULL)
			||*/
			(lastColumnWriter != NULL)
			|| (bestScoreLocation == AT_SEQUENCE_2 || bestScoreLocation == AT_SEQUENCE_1_OR_2)
			|| ((goalScoreLocation == AT_ANYWHERE || goalScoreLocation == AT_SEQUENCE_2 || goalScoreLocation == AT_SEQUENCE_1_OR_2) && lastColumnReader != NULL)
			;
}

/*
 * @see definition on header file
 */
bool AlignerManager::mustDispatchSpecialRows() {
	return specialRowsPartition != NULL && specialRowsPartition->isPersistent();
}

/*
 * @see definition on header file
 */
bool AlignerManager::mustDispatchSpecialColumns() {
	return false;
}

/*
 * @see definition on header file
 */
bool AlignerManager::mustDispatchScores() {
	return (/*processScoreFunction != NULL ||*/ bestScoreLocation == AT_ANYWHERE || goalScoreLocation == AT_ANYWHERE || blocksFile != NULL);
}

/*
 * @see definition on header file
 */
int AlignerManager::getRecurrenceType() const {
	return recurrenceType;
}

/*
 * @see definition on header file
 */
int AlignerManager::getSpecialRowInterval() const {
	return specialRowInterval;
}

int AlignerManager::getSpecialColumnInterval() const {
	return 0;
}

void AlignerManager::stopAligner() {
	active = false;
}

bool AlignerManager::mustContinue() {
	return active;
}


/*
 * @see definition on header file
 */
bool AlignerManager::mustPruneBlocks() {
	return blockPruning;
}

/*
 * @see definition on header file
 */
int AlignerManager::getFirstColumnInitType() {
	return firstColumnInitType;
}

/*
 * @see definition on header file
 */
void AlignerManager::setPenalties(const int match, const int mismatch,
		const int gapOpen, const int gapExtension) {
	// TODO Not supported yet.
	this->match = match;
	this->mismatch = mismatch;
	this->gapOpen = gapOpen;
	this->gapExtension = gapExtension;
}

/*
 * @see definition on header file
 */
int AlignerManager::getFirstRowInitType() {
	return firstRowInitType;
}

void AlignerManager::setBestScoreList(BestScoreList* bestScoreList, const int bestScoreLocation) {
	if (bestScoreList == NULL || bestScoreList == NULL) {
		this->bestScoreLocation = AT_NOWHERE;
		this->bestScoreList = NULL;
	} else {
		this->bestScoreLocation = bestScoreLocation;
		this->bestScoreList = bestScoreList;
	}
}

void AlignerManager::setBlocksFile(BlocksFile* blocksFile) {
	this->blocksFile = blocksFile;
}

void AlignerManager::setGoalScore(int goalScore, const int goalScoreLocation) {
	if (goalScore == -INF || goalScoreLocation == AT_NOWHERE) {
		unsetGoalScore();
	} else {
		this->goalScore = goalScore;
		this->goalScoreLocation = goalScoreLocation;
	}
}

void AlignerManager::unsetGoalScore() {
	this->goalScore = -INF;
	this->goalScoreLocation = AT_NOWHERE;
}

const crosspoint_t AlignerManager::getNextCrosspoint() const {
	if (foundCrosspoint) {
		return nextCrosspoint;
	} else {
		crosspoint_t null;
		null.i = -1;
		null.j = -1;
		null.score = -INF;
		null.type = TYPE_MATCH;
		return null;
	}
}


bool AlignerManager::isFoundCrosspoint() const {
	return foundCrosspoint;
}

int AlignerManager::findBestCell(const cell_t* buffer, int len) {
	int best_score = -INF;
	int best_id = 0;
	for (int k = 0; k < len; k++) {
		if (best_score < buffer[k].h) {
			best_score = buffer[k].h;
			best_id = k;
		}
		if (DEBUG) printf("bestCell: [%d]:%d/%d\n", k, buffer[k].h, best_score);
	}
	return best_id;
}


match_result_t AlignerManager::findGoalCell(const cell_t* buffer, cell_t* base, int len, CellsReader* cellsReader) {
	match_result_t result;
	result.found = false;
	if (!foundCrosspoint) {
		if (cellsReader == NULL) {
			/*if (goalScoreLocation != AT_ANYWHERE) {
				for (int k = 0; k < len; k++) {
					if (buffer[k].h == goalScore) {
						result.found = true;
						result.score = 0;
						result.type = MATCH_ALIGNED;
						result.k = k;
						foundCrosspoint = true;
						break;
					}
				}
			}*/
		} else {
			for (int k=0; k<len; k+=BUS_BASE_SIZE) {
				int local_len = len-k > BUS_BASE_SIZE?BUS_BASE_SIZE:len-k;
				cellsReader->read(base, local_len);
				result = aligner->matchLastColumn ( buffer+k, base, local_len, goalScore );
				if ( result.found ) {
					result.k += k;
					foundCrosspoint = true;
					break;
				}
			}
		}
	}
	return result;
}

match_result_t AlignerManager::findFullGap(int len, bool openGap, SeekableCellsReader* cellsReader) {
	match_result_t result;
	result.found = false;

	if (!foundCrosspoint && cellsReader != NULL) {
		cell_t first_cell;
		first_cell.f = len*-score_params->gap_ext - (openGap ? score_params->gap_open : 0);
		first_cell.h = -INF;
		cell_t base_cell;
		int offset = cellsReader->getOffset();
		cellsReader->read(&base_cell, 1);
		cellsReader->seek(offset);
		if (base_cell.f + first_cell.f + score_params->gap_open == goalScore) {
			result.found = true;
			result.k = len;
			result.score = base_cell.f;
			result.type = MATCH_GAPPED;
			foundCrosspoint = true;
		}

//		int first = -1;
//		int last = -1;
//		for (int k=0; k<len; k++) {
//			cellsReader->read(&base_cell, 1);
//			int gaps = len-k;
//			int min_penalty = (gaps==0) ? 0 : score_params->gap_open+gaps*score_params->gap_ext;
//			//int max_penalty = (gaps==0) ? 0 : (len+k)*score_params->gap_ext;
//			int min_range = k*score_params->mismatch - min_penalty;
//			int max_range = k*score_params->match - min_penalty;
//			int diff = goalScore-base_cell.h;
//			if (diff <= max_range && diff >= min_range) {
////				printf(">>>>>> %d/%d, base: %d   diff: %d   range: %d/%d  %s\n", k, len,
////					base_cell.h, diff, min_range, max_range,
////					(diff <= max_range && diff >= min_range) ? "POSSIBLE!" : "");
//				if (first == -1) first = k;
//				last = k;
//			}
//		}
//		for (int k=len; k<len*2; k++) {
//			cellsReader->read(&base_cell, 1);
//			int gaps = k-len;
//			int min_penalty = (gaps==0) ? 0 : score_params->gap_open+gaps*score_params->gap_ext;
//			//int max_penalty = (gaps==0) ? 0 : (len+k)*score_params->gap_ext;
//			int min_range = len*score_params->mismatch - min_penalty;
//			int max_range = len*score_params->match - min_penalty;
//			int diff = goalScore-base_cell.h;
//			if (diff <= max_range && diff >= min_range) {
////				printf(">>>>>$ %d/%d, base: %d   diff: %d   range: %d/%d  %s\n", k, len,
////					base_cell.h, diff, min_range, max_range,
////					(diff <= max_range && diff >= min_range) ? "POSSIBLE!" : "");
//				if (first == -1) first = k;
//				last = k;
//			}
//		}
//		cellsReader->seek(offset);
//		printf(">>>>>>> RANGE: %d/%d\n", first+offset, last+offset);

	}

	return result;
}

score_t AlignerManager::getBestScoreLastColumn() const {
	return bestScoreLastColumn;
}

score_t AlignerManager::getBestScoreLastRow() const {
	return bestScoreLastRow;
}

