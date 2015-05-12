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

class AbstractAligner;

#ifndef ABSTRACTALIGNER_HPP_
#define ABSTRACTALIGNER_HPP_

#include <string.h>
#include <string>
#include <map>
using namespace std;

//#include "libmasa.hpp"
#include "../IManager.hpp"
#include "../Partition.hpp"
#include "../IAligner.hpp"
#include "../Grid.hpp"
#include "../pruning/AbstractBlockPruning.hpp"

/** @brief Abstract class that executes the %Alignment procedure.
 *
 * The AbstractAligner class is the basic implementation of the interface
 * between the portable code and the non-portable code of MASA (Malleable Architecture
 * for Sequence Aligners). Each MASA extension must create its own Aligner
 * class that extend the AbstractAligner class. The Aligner class has 3 group of methods:
 *
 *
 * <ul>
 *  <li>Public Methods. They are used between the MASA framework and the MASA
 * the public methods;
 *  <li>Virtual methods. They must be implemented by the non-portable code;
 *  <li>Protected methods. Simplifies routines common to all aligners.
 *  <li>Delegate methods. Are methods with protected C++ visibility that may
 *  	be used to obtain or send information between the Aligner subclass and
 *  	the MASA framework.
 * </ul>
 *
 *
 * Although all the abstract methods must be implemented, the Aligner must
 * also be compliant with many requirements in order to produce a proper
 * integration. If the Aligner is fully compliant with a given requirement,
 * we say that the Aligner has that capability. See the aligner_capabilities_t struct
 * to see all the proposed requirements.
 */
class AbstractAligner : public IAligner {
public:
	/* Constructors */

	AbstractAligner();
	virtual ~AbstractAligner();

	/* Implemented virtual methods inherited from IAligner */

	virtual void setManager(IManager* manager);
	virtual const int* getForkWeights();
	virtual match_result_t matchLastColumn(const cell_t* buffer, const cell_t* base, int len, int goalScore);

protected:

	/* Methods to simplify the aligner implementation */

	void setForkCount(const int forkCount, const int* forkWeights = NULL);
	Grid* createGrid(Partition partition);
	virtual const Grid* getGrid() const;
	void initializeBlockPruning(AbstractBlockPruning* blockPruner);

	/* Delegate-pattern to the IManger methods */

	int getRecurrenceType() const;
	int getSpecialRowInterval() const;
	int getSpecialColumnInterval() const;
	int getFirstColumnInitType();
	Partition getSuperPartition();
	int getFirstRowInitType();

	void receiveFirstRow(cell_t* buffer, int len);
	void receiveFirstColumn(cell_t* buffer, int len);
	void dispatchColumn(int j, const cell_t* buffer, int len);
	void dispatchRow(int i, const cell_t* buffer, int len);
	void dispatchScore(score_t score, int bx=-1, int by=-1);

	bool mustContinue();
	bool mustDispatchLastCell();
	bool mustDispatchLastRow();
	bool mustDispatchLastColumn();
	bool mustDispatchSpecialRows();
	bool mustDispatchSpecialColumns();
	bool mustDispatchScores();
	bool mustPruneBlocks();

	cell_t getFirstColumnTail() const;
	cell_t getFirstRowTail() const;

private:
	/** Manager object that receives calls from Aligner to MASA-Core. */
	IManager* manager;

	/** The computational power weights of each forked processes */
	int* forkWeights;

	/** Maximum number of forked proceess */
	int forkCount;

	/** The processing grid */
	Grid* grid;

	/** Last cell read in the first column */
	cell_t firstColumnTail;

	/** Last cell read in the first row */
	cell_t firstRowTail;
};

#endif /* ABSTRACTALIGNER_HPP_ */
