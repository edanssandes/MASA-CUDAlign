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

#ifndef IMANAGER_HPP_
#define IMANAGER_HPP_

#include "libmasaTypes.hpp"
#include "Partition.hpp"

/* Recurrence Functions */

/** NW recurrence function */
#define NEEDLEMAN_WUNSCH	(0)
/** SW recurrence function */
#define SMITH_WATERMAN		(1)

/* Predefined initialization functions */

/** Init row/column with zeros */
#define INIT_WITH_ZEROES		(0)

/** Init row/column with gaps */
#define INIT_WITH_GAPS			(1)

/** Init row/column with gaps and without gap opening penalty*/
#define INIT_WITH_GAPS_OPENED	(3)

/** Init row/column with custom data */
#define INIT_WITH_CUSTOM_DATA	(2)

/* Start type of the partition */

/** Partition starts normally */
#define START_TYPE_MATCH	(0)

/** Partition starts with horizontal gap */
#define START_TYPE_GAP_H	(1)

/** Partition starts with vertical gap */
#define START_TYPE_GAP_V	(2)


/** @brief Interface that manages the MASA extension execution.
 *
 * The MASA framework has an implementation of the IManager interface, that is
 * responsible to receive commands from the Aligner. These commands are
 * classified in the following types:
 *
 * <ul>
 *  <li><b>getXXX</b> methods: Return information about the desired alignment. For
 *  		example, the get methods returns the coordinates of the partition
 *  		to be aligned; the sequences data; the start type of the alignment
 *  <li><b>receiveXXX</b> methods: Receives rows or columns from the MASA framework.
 *  <li><b>dispatchXXX</b> methods: Sends rows or columns to the MASA framework.
 *  <li><b>mustXXX</b> methods: Returns true if some requirement must be activated.
 * </ul>
 *
 * In order to test these conditional requirements, each aligner is associated
 * with an instance of the IManager interface (see setManager() function).
 * Besides the conditional requirement tests, the Manager also provides all
 * the parameters necessary to customize the alignment, for example
 * the sequences, the partitions coordinates.
 * The AbstractAligner hides the IManager invocation using some delegate
 * methods with protected visibility.
 *
 * @see The aligner_capabilities_t struct describes all the possible capabilities
 * and the requirement necessary to implement it.
 * @see The AbstractAligner has many methods that helps the implementation of
 * the IAligner interface.
 * @see The IManager interface manages the execution of the IAligner.
 */
class IManager {
public:

	/* "GET" METHODS */

	/**
	 * @return the recurrence type of the alignment. Possible values are SMITH_WATERMAN and NEEDLEMAN_WUNSCH.
	 */
	virtual int getRecurrenceType() const = 0;

	/**
	 * @return the minimum distance between two special rows, or 0 if it must
	 * not save special rows.
	 */
	virtual int getSpecialRowInterval() const = 0;

	/**
	 * @return the minimum distance between two special columns, or 0 if it must
	 * not save special columns.
	 */
	virtual int getSpecialColumnInterval() const = 0;

	/**
	 * Returns the initialization type of the first column. Possible values are
	 * <ul>
	 *  <li>
	 *  	INIT_WITH_CUSTOM_DATA: the first column must be initialized with custom data
	 *  	that can only be obtained by the AbstractAligner::receiveFirstColumn method.
	 * 	</li>
	 *
	 *  <li>
	 *  	INIT_WITH_GAPS: the first column must be initialized considering gaps.
	 *  	The initialization equation is:
	 *		\f{eqnarray*}{H_{0,0} &=& 0 \\ H_{k,0} &=& -k.G_{ext}-G_{open} \\ F_{k,0} &=& -\infty \f}
	 * 	</li>
	 *
	 *  <li>
	 *  	INIT_WITH_GAPS_OPENED: the first column must be initialized considering gaps,
	 *  	but without gap opening penalty.
	 *  	The initialization equation is:
	 * 		\f{eqnarray*}{H_{k,0} &=& -k.G_{ext} \\ F_{k,0} &=& -\infty \f}
	 * 	</li>
	 *
	 *  <li>
	 *  	INIT_WITH_ZEROES: the first column must be initialized considering zero values.
	 *  	The initialization equation is:
	 *  	\f{eqnarray*}{H_{k,0} &=& 0 \\ F_{k,0} &=& -\infty \f}
	 * 	</li>
	 *
	 * The initialization data of all types may be obtained by the AbstractAligner::receiveFirstColumn
	 * method, but the subclass of AbstractAligner may implement the initialization functions
	 * using some architectural dependent code (for example, using vectorial hardware instructions).
	 *
	 * </ul>
	 * @return the initialization type.
	 */
	virtual int getFirstColumnInitType() = 0;

	/**
	 * Returns the initialization type of the first row. Possible values are
	 * <ul>
	 *  <li>
	 *  	INIT_WITH_CUSTOM_DATA: the first row must be initialized with custom data
	 *  	that can only be obtained by the AbstractAligner::receiveFirstRow method.
	 * 	</li>
	 *
	 *  <li>
	 *  	INIT_WITH_GAPS: the first column must be initialized considering gaps.
	 *  	The initialization equation is:
	 *		\f{eqnarray*}{H_{0,0} &=& 0 \\ H_{0,k} &=& -k.G_{ext}-G_{open} \\ F_{0,k} &=& -\infty \f}
	 * 	</li>
	 *
	 *  <li>
	 *  	INIT_WITH_GAPS_OPENED: the first column must be initialized considering gaps,
	 *  	but without gap opening penalty.
	 *  	The initialization equation is:
	 * 		\f{eqnarray*}{H_{0,k} &=& -k.G_{ext} \\ F_{0,k} &=& -\infty \f}
	 * 	</li>
	 *
	 *  <li>
	 *  	INIT_WITH_ZEROES: the first row must be initialized considering zero values.
	 *  	The initialization equation is:
	 *  	\f{eqnarray*}{H_{0,k} &=& 0 \\ F_{0,k} &=& -\infty \f}
	 * 	</li>
	 *
	 * The initialization data of all types may be obtained by the AbstractAligner::receiveFirstRow
	 * method, but the subclass of AbstractAligner may implement the initialization functions
	 * using some architectural dependent code (for example, using vectorial hardware instructions).
	 *
	 * </ul>
	 * @return the initialization type.
	 */
	virtual int getFirstRowInitType() = 0;



	/**
	 * Returns the super partition that includes all sub partitions being aligned.
	 * This method must be used only by block pruning algorithms in order to
	 * obtain the corner coordinates of the matrix.
	 *
	 * A super partition is a set of many smaller partitions and this occurs
	 * in two situations. The first is when we are running stage1 in a
	 * multiprocess environment, where we have one sub partition for each
	 * process. The second situation happens when the aligner cannot handle
	 * the full size of the matrix, so the partition is split in parts
	 * smaller than the max sequence size capability of the aligner.
	 *
	 * @return the super partition.
	 */
	virtual Partition getSuperPartition() = 0;


	/* "RECEIVE" METHODS */

	/**
	 * Receives the first row of the partition. This function may block until
	 * all the requested data is ready. So prefer to read data in chunks instead
	 * of reading the full first row.
	 * The data will be stored from 0 to len-1 positions of the vector passed in the parameters.
	 *
	 * @param buffer the vector where the first row data will be stored.
	 * @param len the number of cells that will be read.
	 */
	virtual void receiveFirstRow(cell_t* buffer, int len) = 0;

	/**
	 * Receives the first column of the partition. This function may block until
	 * all the requested data is ready. So prefer to read data in chunks instead
	 * of reading the full first row.
	 * The data will be stored from 0 to len-1 positions of the vector passed in the parameters.
	 *
	 * @param buffer the vector where the first column data will be stored.
	 * @param len the number of cells that will be read.
	 */
	virtual void receiveFirstColumn(cell_t* buffer, int len) = 0;

	/* "DISPATCH" METHODS */

	/**
	 * Notifies to the MASA framework that some cells of a column has been processed.
	 * This function must be called serially for each column. For example,
	 * the invocation of dispatchColumn(50000, vector, 100) will dispatch the
	 * first 100 cells of the column 50000 to MASA, and the cells are read from
	 * the vector[0..99] elements. After this, a call to dispatchColumn(50000, vector, 50)
	 * will dispatch the next 50 cells of the same column 5000, and the cells are read from
	 * the vector[0..49] elements.
	 *
	 * @param j			the column to be dispatched.
	 * @param buffer	the vector containing the data (starting from cell 0).
	 * @param len		the number of cells that will be read from the vector.
	 */
	virtual void dispatchColumn(int j, const cell_t* buffer, int len) = 0;

	/**
	 * Notifies to the MASA framework that some cells of a row has been processed.
	 * This function must be called serially for each row, analogous to the
	 * AbstractAligner::dispatchLastColumn method.
	 *
	 * @param i			the row to be dispatched.
	 * @param buffer	the vector containing the data (starting from cell 0).
	 * @param len		the number of cells that will be read from the vector.
	 */
	virtual void dispatchRow(int i, const cell_t* buffer, int len) = 0;

	/**
	 * Notifies to the MASA framework that a new score has been computed.
	 * This method may be called as many times it is necessary, and the
	 * best score will be calculated among all calls of this method.
	 *
	 * If the Aligner supports the aligner_capabilities_t::dispatch_block_scores
	 * capability, them it must dispatch the score with the bx, by parameters
	 * set to the block indices and this method must be called only once for
	 * each block.
	 *
	 * @param score	the score to be dispatched
	 * @param bx	the block position in the horizontal direction, starting from
	 * 				0 up to AbstractAligner::getGridWidth() minus 1.
	 * @param by	the block position in the vertical direction, starting from
	 * 				0 up to AbstractAligner::getGridHeight() minus 1.
	 */
	virtual void dispatchScore(score_t score, int bx=-1, int by=-1) = 0;

	/* "MUST" METHODS */

	/**
	 * @return true if the execution must continue.
	 */
	virtual bool mustContinue() = 0;

	/**
	 * @return true if the last cell must be dispatched.
	 */
	virtual bool mustDispatchLastCell() = 0;

	/**
	 * @return true if the last row must be dispatched.
	 */
	virtual bool mustDispatchLastRow() = 0;

	/**
	 * @return true if the last column must be dispatched.
	 */
	virtual bool mustDispatchLastColumn() = 0;

	/**
	 * @return true if special rows must be dispatched.
	 */
	virtual bool mustDispatchSpecialRows() = 0;

	/**
	 * @return true if special columns must be dispatched.
	 */
	virtual bool mustDispatchSpecialColumns() = 0;

	/**
	 * @return true if intermediate scores must be dispatched.
	 */
	virtual bool mustDispatchScores() = 0;

	/**
	 * @return true if block pruning optimization may be used.
	 */
	virtual bool mustPruneBlocks() = 0;

protected:
/* Avoid the creation/deletion of this interface */
		~IManager() {};
		IManager() {};
};


#endif /* IMANAGER_HPP_ */
