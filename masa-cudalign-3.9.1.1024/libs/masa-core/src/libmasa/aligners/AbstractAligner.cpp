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

#include "AbstractAligner.hpp"

#include <stdio.h>
#include <stdlib.h>

#include "../utils/AlignerUtils.hpp"
#include "../pruning/BlockPruningGenericN2.hpp"

/**
 * AbstractAligner contructor.
 */
AbstractAligner::AbstractAligner() {
	forkWeights = NULL;
	forkCount = 0;
	grid = NULL;
	manager = NULL;

	firstColumnTail.h = -INF;
	firstColumnTail.f = -INF;
	firstRowTail.h = -INF;
	firstRowTail.f = -INF;
}

/**
 * AbstractAligner destructor.
 */
AbstractAligner::~AbstractAligner() {
	if (forkWeights != NULL) {
		delete forkWeights;
	}
}


/**
 * Defines the IManager to be associated with this aligner.
 * @param manager the interface to the MASA-Core.
 */
void AbstractAligner::setManager(IManager* manager) {
	this->manager = manager;
}

/**
 * Defines how many processes may be forked for this aligner and the
 * computation weight for each process.
 *
 * @param forkCount number of processes allowed.
 * @param forkWeights vector containing $forkCount$ weights. If NULL, all
 * the processes will have the same weight.
 */
void AbstractAligner::setForkCount(const int forkCount, const int* forkWeights) {
	this->forkCount = forkCount;
	if (this->forkWeights != NULL) {
		// forkWeights will be recreated.
		delete this->forkWeights;
	}
	this->forkWeights = new int[forkCount+1];
	if (forkWeights != NULL) {
		memcpy(this->forkWeights, forkWeights, sizeof(int)*forkCount);
		this->forkWeights[forkCount] = 0;
	} else {
		// All the weigths are set to the same number, so the processes
		// will be equally distributed.
		for (int i=0; i<forkCount; i++) {
			this->forkWeights[i] = 100;
		}
	}
}

/**
 * Supply the computational power weight defined after the AbstractAligner::setForkCount.
 *
 * @return the weights for each process.
 * @see IAligner::getForkWeights()
 */
const int* AbstractAligner::getForkWeights() {
	return forkWeights;
}

/**
 * Creates a new grid using the given partition coordinates. If there is
 * a previously created grid, it is deleted and overwritten.
 *
 * @param partition the partition to be split in a grid of blocks.
 */
Grid* AbstractAligner::createGrid(Partition partition) {
	if (this->grid != NULL) {
		delete this->grid;
	}
	this->grid = new Grid(partition);

	return this->grid;
}

/**
 * Returns the grid created by the last call to the AbstractAligner::createGrid
 * method.
 *
 * @return the current grid object.
 */
const Grid* AbstractAligner::getGrid() const {
	return grid;
}

/**
 * Initializes the pruner object. It must be called after the grid is created
 * by the AbstractAligner::createGrid method.
 *
 * @param blockPruner the pruner object to be initialized.
 */
void AbstractAligner::initializeBlockPruning(AbstractBlockPruning* blockPruner) {
	if (this->grid == NULL) {
		fprintf(stderr, "The grid is not set. Block pruning cannot be initialized.\n");
		return;
	}
	if (mustPruneBlocks()) {
		blockPruner->setGrid(grid);
		blockPruner->setSuperPartition(manager->getSuperPartition());
		blockPruner->setScoreParams(getScoreParameters());
		blockPruner->setRecurrenceType(this->getRecurrenceType());
	} else {
		blockPruner->setGrid(NULL);
	}
}

/**
 * Executes the matching procedure in CPU. This method is a default
 * implementation that works in any condition, but a subclass may override it
 * in order to create a faster matching procedure for the MASA extension.
 *
 * @param buffer the vector with the last column data.
 * @param base the vector with the special row in the reverse direction.
 * @param len Defines that we must match the buffers in the range [0,len).
 * @param goalScore the score that will be searched during the matching procedure.
 * @return the match result.
 * @see IAligner::matchLastColumn for a better explanation of the parameters.
 */
match_result_t AbstractAligner::matchLastColumn(const cell_t* buffer,
		const cell_t* base, int len, int goalScore) {
	// Executes the matching procedure.
	return AlignerUtils::matchColumn(buffer, base, len, goalScore,
			getScoreParameters()->gap_open);
}




/* **************** *
 * DELEGATE METHODS *
 * **************** */




/** Delegates to IManager::getRecurrenceType()
 * @copydoc IManager::getRecurrenceType
 * @see IManager::getRecurrenceType()
 */
int AbstractAligner::getRecurrenceType() const {
	return this->manager->getRecurrenceType();
}

/** Delegates to IManager::getSpecialRowInterval()
 * @copydoc IManager::getSpecialRowInterval
 * @see IManager::getSpecialRowInterval()
 */
int AbstractAligner::getSpecialRowInterval() const {
	return this->manager->getSpecialRowInterval();
}

/** Delegates to IManager::getSpecialColumnInterval()
 * @copydoc IManager::getSpecialColumnInterval
 * @see IManager::getSpecialColumnInterval()
 */
int AbstractAligner::getSpecialColumnInterval() const {
	return this->manager->getSpecialColumnInterval();
}

/** Delegates to IManager::getSuperPartition()
 * @copydoc IManager::getSuperPartition
 * @see IManager::getSuperPartition()
 */
Partition AbstractAligner::getSuperPartition() {
	return this->manager->getSuperPartition();
}

/** Delegates to IManager::getFirstColumnInitType()
 * @copydoc IManager::getFirstColumnInitType
 * @see IManager::getFirstColumnInitType()
 */
int AbstractAligner::getFirstColumnInitType() {
	return this->manager->getFirstColumnInitType();
}

/** Delegates to IManager::getFirstRowInitType()
 * @copydoc IManager::getFirstRowInitType
 * @see IManager::getFirstRowInitType()
 */
int AbstractAligner::getFirstRowInitType() {
	return this->manager->getFirstRowInitType();
}

/** Delegates to IManager::receiveFirstRow()
 * @copydoc IManager::receiveFirstRow
 * @see IManager::receiveFirstRow()
 */
void AbstractAligner::receiveFirstRow(cell_t* buffer, int len) {
	this->manager->receiveFirstRow(buffer, len);
	firstRowTail = buffer[len - 1];
}

/** Delegates to IManager::receiveFirstColumn()
 * @copydoc IManager::receiveFirstColumn
 * @see IManager::receiveFirstColumn()
 */
void AbstractAligner::receiveFirstColumn(cell_t* buffer, int len) {
	this->manager->receiveFirstColumn(buffer, len);
	firstColumnTail = buffer[len - 1];
}


/** Delegates to IManager::dispatchColumn()
 * @copydoc IManager::dispatchColumn
 * @see IManager::dispatchColumn()
 */
void AbstractAligner::dispatchColumn(int j, const cell_t* buffer, int len) {
	this->manager->dispatchColumn(j, buffer, len);
}

/** Delegates to IManager::dispatchRow()
 * @copydoc IManager::dispatchRow
 * @see IManager::dispatchRow()
 */
void AbstractAligner::dispatchRow(int i, const cell_t* buffer, int len) {
	this->manager->dispatchRow(i, buffer, len);
}

/** Delegates to IManager::dispatchScore()
 * @copydoc IManager::dispatchScore
 * @see IManager::dispatchScore()
 */
void AbstractAligner::dispatchScore(score_t score, int bx, int by) {
	this->manager->dispatchScore(score, bx, by);
}

/** Delegates to IManager::mustContinue()
 * @copydoc IManager::mustContinue
 * @see IManager::mustContinue()
 */
bool AbstractAligner::mustContinue() {
	return this->manager->mustContinue();
}

/** Delegates to IManager::mustDispatchLastCell()
 * @copydoc IManager::mustDispatchLastCell
 * @see IManager::mustDispatchLastCell()
 */
bool AbstractAligner::mustDispatchLastCell() {
	return this->manager->mustDispatchLastCell();
}

/** Delegates to IManager::mustDispatchLastRow()
 * @copydoc IManager::mustDispatchLastRow
 * @see IManager::mustDispatchLastRow()
 */
bool AbstractAligner::mustDispatchLastRow() {
	return this->manager->mustDispatchLastRow();
}

/** Delegates to IManager::mustDispatchLastColumn()
 * @copydoc IManager::mustDispatchLastColumn
 * @see IManager::mustDispatchLastColumn()
 */
bool AbstractAligner::mustDispatchLastColumn() {
	return this->manager->mustDispatchLastColumn();
}

/** Delegates to IManager::mustDispatchSpecialRows()
 * @copydoc IManager::mustDispatchSpecialRows
 * @see IManager::mustDispatchSpecialRows()
 */
bool AbstractAligner::mustDispatchSpecialRows() {
	return this->manager->mustDispatchSpecialRows();
}

/** Delegates to IManager::mustDispatchScores()
 * @copydoc IManager::mustDispatchScores
 * @see IManager::mustDispatchScores()
 */
bool AbstractAligner::mustDispatchScores() {
	return this->manager->mustDispatchScores();
}

/** Delegates to IManager::mustDispatchSpecialColumns()
 * @copydoc IManager::mustDispatchSpecialColumns
 * @see IManager::mustDispatchSpecialColumns()
 */
bool AbstractAligner::mustDispatchSpecialColumns() {
	return this->manager->mustDispatchSpecialColumns();
}

/** Delegates to IManager::mustPruneBlocks()
 * @copydoc IManager::mustPruneBlocks
 * @see IManager::mustPruneBlocks()
 */
bool AbstractAligner::mustPruneBlocks() {
	return this->manager->mustPruneBlocks();
}

/**
 * @return the last cell read from the first column
 */
cell_t AbstractAligner::getFirstColumnTail() const {
	return firstColumnTail;
}

/**
 * @return the last cell read from the first row
 */
cell_t AbstractAligner::getFirstRowTail() const {
	return firstRowTail;
}


