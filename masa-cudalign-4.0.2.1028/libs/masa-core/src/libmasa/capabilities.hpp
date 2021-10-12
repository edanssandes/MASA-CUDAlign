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

#ifndef CAPABILITIES_HPP_
#define CAPABILITIES_HPP_

/**
 * Use this constant for the unsupported capabilities by the Aligner.
 */
#define NOT_SUPPORTED	(0)

/**
 * Use this constant for the supported capabilities by the Aligner.
 */
#define SUPPORTED		(1)

/** @brief Struct that informs to the MASA framework which capabilities
 * the MASA extension implements.
 *
 * It is a development choice to implement or not all the capabilities,
 * but there are some MASA features that will not work if some capability
 * is missing (or if it is incorrectly implemented).
 *
 * Each capability is associated with a requirement of the IAligner interface.
 * If the IAligner implementation is fully compliant with a given requirement,
 * we say that it implements that capability. In this case, set the
 * boolean attributes to SUPPORTED, otherwise set it to NOT_SUPPORTED.
 *
 * Note that even if the IAligner implements a given capability, it must only
 * execute that capability if some conditional test is true.
 * Each capability has its own conditional requirement that may be obtained
 * by the associated IManager interface (see IAligner::setManager) or by the
 * AbstractAligner protected methods.
 *
 * Besides the boolean attributes, there are other parameters that indicates
 * limits of other aligner properties.
 *
 * See the description of each capability in this page, with its requirements
 * and conditional tests.
 */
typedef struct aligner_capabilities_t {
	/**
	 * Initializes the first column with custom data. The customized column
	 * data is obtained by the IManager::receiveFirstColumn method.
	 *
	 * <b>condition:</b> <tt>always true</tt>
	 */
	bool customize_first_column;

	/**
	 * Initializes the first row with custom data. The customized column
	 * data is obtained by the IManager::receiveFirstRow method.
	 *
	 * <b>condition:</b> <tt>always true</tt>
	 */
	bool customize_first_row;

	/**
	 * Dispatches the last column of the partition using the
	 * IManager::dispatchColumn method.
	 *
	 * <b>condition:</b> <tt>IManager::mustDispatchLastColumn() == true</tt>
	 */
	bool dispatch_last_column;

	/**
	 * Dispatches the last row of the partition using the
	 * IManager::dispatchRow method.
	 *
	 * <b>condition:</b> <tt>IManager::mustDispatchLastRow() == true</tt>
	 */
	bool dispatch_last_row;

	/**
	 * Dispatches the last cell of the partition using the
	 * IManager::dispatchScore method.
	 *
	 * <b>condition:</b> <tt>IManager::mustDispatchLastCell() == true</tt>
	 */
	bool dispatch_last_cell;

	/**
	 * Dispatches special columns using the IManager::dispatchColumn method.
	 * The distance between two special columns must be greater than
	 * IManager::getSpecialColumnInterval.
	 *
	 * <b>condition:</b> <tt>IManager::mustDispatchSpecialColumns() == true</tt>
	 */
	bool dispatch_special_column;

	/**
	 * Dispatches special rows using the IManager::dispatchRow method.
	 * The distance between two special rows must be greater than
	 * IManager::getSpecialRowInterval.
	 *
	 * <b>condition:</b> <tt>IManager::mustDispatchSpecialRows() == true</tt>
	 */
	bool dispatch_special_row;

	/**
	 * Dispatches the best score using the IManager::dispatchScore method.
	 * There is no problem to call IManager::dispatchScore many times, since
	 * the best best score be dispatched in one of these calls.
	 *
	 * <b>condition:</b> <tt>IManager::mustDispatchScores() == true</tt>
	 */
	bool dispatch_best_score;

	/**
	 * Dispatches more than one score using the IManager::dispatchScore method.
	 * If the method IManager::dispatchScore is called more than one time, set
	 * this capability to <tt>SUPPORTED</tt>. If the method
	 * IManager::dispatchScore is called only once in the end of the partition,
	 * set this capability to <tt>NOT_SUPPORTED</tt>.
	 *
	 * <b>condition:</b> <tt>IManager::mustDispatchScores() == true</tt>
	 */
	bool dispatch_scores;

	/**
	 * Dispatches scores in a regular block pattern. The matrix must
	 * be divided in \f$m\times{}n\f$ blocks and the best score of each block
	 * must be dispatched using the IManager::dispatchScore with the
	 * \f$(bx,by)\f$ block coordinate parameters.
	 *
	 * <b>condition:</b> <tt>IManager::mustDispatchScores() == true</tt>
	 */
	bool dispatch_block_scores;

	/**
	 * Aligns only the region delimited by the partition \f$(i0,j0)-(i1,j1)\f$.
	 * Use methods IManager::getI0(), IManager::getJ0(), IManager::getI1()
	 * and IManager::getJ1() to obtain the partition boundaries.
	 *
	 * <b>condition:</b> <tt>always true</tt>
	 */
	bool process_partition;

	/**
	 * Aligns using variable penalties. The variable penalties must be returned
	 * by the IAligner::getScoreParameters() method. If the parameters are
	 * constants, set this capability to <tt>NOT_SUPORTED</tt>.
	 *
	 * <b>condition:</b> <tt>always true</tt>
	 */
	bool variable_penalties;

	/**
	 * Implements the block pruning optimization. If the method
	 * IManager::dispatchScore() is called for a pruned block, its best score
	 * must be set to \f$-\infty\f$. Moreover, the last row and last column
	 * of a pruned block must be dispatched considering \f$-\infty\f$ cell
	 * values.
	 *
	 * <b>condition:</b> <tt>IManager::mustPruneBlocks() == true</tt>
	 */
	bool block_pruning;

	/**
	 * Aligns with the Needleman Wunsch (NW) recurrence function.
	 *
	 * \f[	 H_{i,j}=\max\{ 0, E_{i,j}, F_{i,j}, H_{i-1,j-1} - p(i,j) \} \f]
	 *
	 * <b>condition:</b> <tt>IManager::getRecurrenceType() == NEEDLEMAN_WUNSCH</tt>
	 */
	bool needleman_wunsch;

	/**
	 * Aligns with the Smith wWaterman (SW) recurrence function.
	 *
	 * \f[	 H_{i,j}=\max\{ 0, E_{i,j}, F_{i,j}, H_{i-1,j-1} - p(i,j) \} \f]
	 *
	 * <b>condition:</b> <tt>IManager::getRecurrenceType() == SMITH_WATERMAN</tt>
	 */
	bool smith_waterman;

	/**
	 * Allows the execution of many forked processes in parallel. The aligner
	 * must return the weights of each forked process in the
	 * IAligner::getForkWeights() methods. Moreover, the aligner may use
	 * the AbstractAlignerParameters::getForkId() to identify uniquely each
	 * process and set different resources for each one of them.
	 *
	 * <b>condition:</b> <tt>AbstractAlignerParameters::getForkId() != NOT_FORKED_PROCESS</tt>
	 */
	bool fork_processes;

	/**
	 * Indicates the maximum allowed size for the vertical sequence (seq0).
	 * Zero-value means unlimited size.
	 */
	int maximum_seq0_len;

	/**
	 * Indicates the maximum allowed size for the horizontal sequence (seq1)
	 * Zero-value means unlimited size.
	 */
	int maximum_seq1_len;

	/**
	 * Constructor that creates a new struct with all capabilities set to
	 * NOT_SUPPORTED
	 */
	aligner_capabilities_t() {
		memset(this, 0, sizeof(aligner_capabilities_t));
	}
} aligner_capabilities_t;


#endif /* CAPABILITIES_HPP_ */
