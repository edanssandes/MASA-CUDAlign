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

#ifndef IALIGNER_HPP_
#define IALIGNER_HPP_

#include <string.h>
#include <stdio.h>

#include "libmasaTypes.hpp"
#include "IManager.hpp"
#include "parameters/AbstractAlignerParameters.hpp"
#include "capabilities.hpp"
#include "Grid.hpp"
#include "Partition.hpp"


/** @brief Interface between the MASA extension and the MASA framework.
 *
 * The IAligner is a pure abstract class that makes an interface point between
 * the portable code of MASA (MASA-Core) and the non-portable code
 * (MASA extension). Each MASA extension must contain its own Aligner, which
 * must implements the IAligner interface for a successful integration.
 *
 * Instead of implementing the IAligner directly, we recommend that the Aligner
 * extend the AbstractAligner class or one of its subclasses, which already
 * has some implemented methods that simplifies the construction of
 * a new IAligner implementation.
 *
 * The subclasses of the AbstractAligner have already implemented some
 * common code used in some kind of alignments. For instance, the
 * AbstractBlockAligner divides the Dynamic Program Matrix in blocks, the
 * OpenMPAligner class processes blocks using OpenMP and the
 * AbstractDiagonalAligner processes the blocks per anti-diagonal. See the
 * documentation of these classes to see the their benefits and utilization.
 *
 * \section sec_lc Aligner's life cycle
 *
 * The MASA-Core uses a single Aligner object during all the comparison, thus
 * it is important to understand the life cycle of this object.
 *
 * 1. <b>Instantiation</b>: The Aligner object is created in the C-main entry point
 *    and it is passed in the libmasa_entry_point() function call. Inside the
 *    constructor, the aligner should initialize all the data structure that
 *    will be used during all the lifecycle. In this moment, there is
 *    absolutely no information about the arguments supplied by the
 *    user.
 *
 * 2. <b>Initialization</b>: At this point, the MASA-Core has already read the command
 *    line arguments and it may have already forked many processes. So, the
 *    IAligner::initialize() method is called for each process with the
 *    unique identification of this process. This identification may be use,
 *    for instance, to initialize the hardware dedicated to this process. See
 *    the AbstractAlignerParameters::getForkId().
 *
 * 3. <b>Stage Execution</b>: On every stage, the MASA-Core change sequence
 *    orientation and align one or more partitions.
 *
 *    3.1. <b>%Sequence Configuration</b>: On every stage, the MASA-Core
 *    defines the orientation of the sequences and the range of nucleotides
 *    that may be processed during the stage. So, each new stage generate
 *    a call to the IAligner::setSequences() notifying the aligner with
 *    the sequence (possibly trimmed) and its maximum accessible length.
 *    Using this information, the aligner may allocate the sequence related
 *    structures using the correct size and data.
 *
 *    3.2. <b>%Partition %Alignment</b>: Each stage may process one or more partitions
 *    to be aligned, where each of them are associated to one call for the
 *    IAligner::alignPartition() method. Each partition is guaranteed to reside inside the sequence
 *    length supplied by IAligner::setSequences(). The
 *    calls to the IAligner::alignPartition() method are done serially,
 *    but inside this method the Aligner should used parallelism in order
 *    to speedup computation. See the IAligner::alignPartition() documentation
 *    in order to understand how to compute a partition.
 *
 *    3.3. <b>%Sequence Deallocation</b>: After each stage, the MASA-Core calls
 *    IAligner::unsetSequences() to notify the aligner for deallocation of
 *    the sequence related structures.
 *
 * 4. <b>Finalization</b>: This is done only once in the end of the process.
 *    This method should be used to deallocate any structure previously allocated
 *    during the Initialization step.
 *
 * \section Statistics
 *
 * The MASA-Core expects that some statistics are collected by the Aligner.
 * For instance, the number of processed cells is used to estimate the
 * GCUPS performance of the Aligner. Furthermore, some string are logged
 * in some files, so the Aligner can print internal information in these logs.
 *
 *
 * \section Capabilities
 *
 * Although the Aligner must implemented all the virtual methods (extending
 * one of the AbstractAligner subclasses or implementing directly the
 * IAligner class), the Aligner must also be compliant with
 * some requirements in order to produce a proper integration.
 * If the Aligner is fully compliant with a given requirement,
 * we say that this Aligner implements a capability. A MASA-Extension is
 * expected to implement a list of capability, which can be seen in the
 * aligner_capabilities_t structure documentation.
 *
 * The MASA-Core call the aligner many times during the execution and, in each
 * invocation, it may require (or not) a list of capabilities. For instance,
 * MASA-Core may require the Smith Waterman (SW) capability in stage 1 and
 * require the Needleman Wunsch (NW) capability in stage 2. Even if the Aligner
 * implements both SW and NW capabilities, the Aligner may execute each
 * capabilities only when requested, otherwise the integration may fail.
 *
 * Each capability is associated with a conditional requirement test that
 * must be verified before its execution. In order to test these conditional
 * requirements, each aligner must call some methods from the IManager
 * interface (see setManager() function). Besides the conditional requirement
 * tests, the Manager also provides all the parameters necessary to customize
 * the alignment, for example the sequences, the partitions coordinates.
 * The AbstractAligner hides the IManager invocation using some delegate
 * methods with protected visibility.
 *
 * The MASA-Core may fork many processes to work in parallel. Furthermore,
 * MASA-Core executes a load balancing considering the computation power of
 * each process. The maximum number of forked processes and its computation
 * power is architectural dependent and is informed by the MASA-Extension
 * using the IAlignerParameters::getForkWeights() method.
 *
 * @see The aligner_capabilities_t struct describes all the possible capabilities
 * and the requirement necessary to implement it.
 * @see The AbstractAligner has many methods that helps the implementation of
 * the IAligner interface.
 * @see The IManager interface manages the execution of the IAligner.
 */
class IAligner {

public:

		/**
		 * Returns the capabilities of the aligner.
		 *
		 * @return the capabilities.
		 * @see aligner_capabilities_t
		 */
		virtual aligner_capabilities_t getCapabilities() = 0;

		/**
		 * Associates this IAligner with an instance of IManager. The IManager
		 * controls the execution of the aligner.
		 *
		 * @param manager the IManager that will control the execution of
		 * this IAligner.
		 */
		virtual void setManager(IManager* manager) = 0;

		/**
		 * Supply the computational power weight of each forked processed.
		 *
		 * The returned vector must contain the weight of each process and it
		 * must be terminated with a 0 element. The amount of the matrix
		 * processed by each process will be determined by the ratio between
		 * each weight and the sum of the weights.
		 *
		 * For example, the vector \f$\{10,20,20,0\}\f$ will
		 * allow the MASA framework to fork 3 processes. The first process
		 * will process \f$\frac{10}{50} = 20\%\f$ of the matrix and the
		 * other two processes will process \f$\frac{20}{50} = 40\%\f$ of
		 * the matrix each.
		 *
		 * @return an integer vector returning the weights of each process.
		 * The last element must be zero. If NULL is returned, no forked
		 * processes will be allowed.
		 */
		virtual const int* getForkWeights() = 0;

		/**
		 * Get the command line parameters of the IAligner class. The
		 * IAlignerParameters interface is used by MASA to present
		 * extra command line parameters to each IAligner subclass. Be
		 * warned that the MASA-Core is responsible to present
		 * all the command line options, so, any attempt to modify
		 * the command line parameters must be done by the
		 * IAlignerParameters class, otherwise the behavior of the
		 * entire MASA-Core may be compromised. The AbstractAlignerParameters
		 * implements the base operations of the IAlignerParameters
		 * interface.
		 *
		 *
		 * @return The customized parameters for this IAligner.
		 *
		 * @see The IAlignerParameters class presents the details
		 * to customize these parameters.
		 */
		virtual IAlignerParameters* getParameters() = 0;

		/**
		 * Returns the match/mismatch parameters and the gap penalties used
		 * by this IAligner.
		 * @return the score parameters of this IAligner.
		 */
		virtual const score_params_t* getScoreParameters() = 0;

		/**
		 * Initializes the Aligner before the execution of the alignment
		 * procedure. The IManager associated with this IAligner may only
		 * be called to obtain the command line parameters, specially the
		 * AbstractAlignerParameters::getForkId() in multi-process executions.
		 *
		 * The IManager is not set and must not be queried. The initialize()
		 * method is called only once per process. Here, we may initialize
		 * the hardware and allocate some global structures that are not
		 * associated with the sequence sizes.
		 *
		 *
		 * The initialize() method will be called once for each MASA stage and
		 * the sequences will not be changed until the finalize method be called.
		 * Meanwhile, the alignPartition() method may be called multiple times
		 * before the finalize() method is called.
		 *
		 * The initialize() method may be used to process and allocated the
		 * sequences in memory. Note that the MASA stages may change the
		 * direction of the sequences, so consider that each call to the
		 * initialize method will change the sequence data.
		 */
		virtual void initialize() = 0;

		/**
		 * This method is called in the beginning of each stage to inform
		 * the aligner about the sequence to be aligned. The MASA stages
		 * alternates the direction of the sequences in each
		 * stage, possibly trimming if the beginning and end of the sequences
		 * will not be used in this stage. So consider that each call to
		 * the onSequenceChange() method may completely change the sequence
		 * data for the further calls to alignPartition.
		 *
		 * Note that the seq0_len and seq1_len parameters are not
		 * the sizes of the original sequences, but the sizes of the trimmed
		 * sequences.
		 *
		 * @param seq0	trimmed vertical sequence data
		 * @param seq1	trimmed  horizontal sequence data
		 * @param seq0_len	length of the trimmed vertical sequence.
		 * @param seq1_len	length of the trimmed horizontal sequence.
		 */
		virtual void setSequences(const char* seq0, const char* seq1, int seq0_len, int seq1_len) = 0;

		/**
		 * Defines that the sequence will not be used anymore and the Aligner
		 * should deallocate the memory used for them. This method is called
		 * in the end of each stage.
		 */
		virtual void unsetSequences() = 0;

		/**
		 * Executes the alignment procedure.
		 *
		 * During the call of this method, all the methods of the IManager
		 * can be called to obtain the alignment parameters (partition
		 * boundaries, row/column data, conditional requirements, etc).
		 * Note that the sequence data is already available
		 * during the initialize() invocation,
		 * but the other information is only available during the invocation of
		 * the alignPartition() method.
		 *
		 * The alignPartition() method may be called multiple
		 * times between onSequenceChange() method calls.
		 *
		 * @param partition the partition to be aligned.
		 */
		virtual void alignPartition(Partition partition) = 0;


		/**
		 * Finalizes the execution of this IAligner. Use this method to free
		 * any allocated memory during the life time of the IAligner.
		 */
		virtual void finalize() = 0;

		/**
		 * This method executes the Myers-Miller matching procedure.
		 *
		 * @param buffer the vector with the last column data.
		 * @param base the vector with the special row in the reverse direction.
		 * 			This vector is the special row computed in the previous stage.
		 * @param len Defines that we must match the buffers in the range [0,len).
		 * @param goalScore the score that will be searched during the matching procedure.
		 * @return A match_result_t struct. If match_result_t::found is false, than
		 * 			the match procedure did not find the goal score. Otherwise,
		 * 			match_result_t::found is true and match_result_t::i and match_result_t::j
		 * 			contains the coordinate where the goal score was found. Additionally,
		 * 			match_result_t::type may be a MATCH_ALIGNED if the goal was found in
		 * 			the \f$H\f$ (match) boundary or MATCH_GAPPED if it was found in the \f$F\f$ (gap) boundary.
		 * 			If both MATCH_ALIGNED and MATCH_GAPPED applies, the MATCH_ALIGNED must be preferred.
		 */
		virtual match_result_t matchLastColumn(const cell_t* buffer, const cell_t* base, int len, int goalScore) = 0;

		/**
		 * Returns the grid of blocks. This method is only necessary if
		 * the capabilities_t::dispatch_block_scores is SUPPORTED.
		 *
		 * @return the number of blocks in the vertical direction of the grid.
		 */
		virtual const Grid* getGrid() const = 0;

	/* Statistic functions */

		/**
		 * clear all internal statistics of the aligner.
		 */
		virtual void clearStatistics() = 0;

		/**
		 * This method is called immediately after initialize(), allowing
		 * the aligner to print some initial information.
		 *
		 * @param file The log file where the statistics will be written.
		 */
		virtual void printInitialStatistics(FILE* file) = 0;

		/**
		 * This method is called immediately after onSequenceChange(), allowing
		 * the aligner to print some information before a new stage.
		 *
		 * @param file The log file where the statistics will be written.
		 */
		virtual void printStageStatistics(FILE* file) = 0;

		/**
		 * This method is called immediately after finalize(), allowing
		 * the aligner to print some finalization information.
		 *
		 * @param file The log file where the statistics will be written.
		 */
		virtual void printFinalStatistics(FILE* file) = 0;

		/**
		 * This method allows the aligner to print the internal statistics,
		 * considering that they ware cleaned in the last call of
		 * clearStatistics() method.
		 *
		 * @param file The log file where the statistics will be written.
		 */
		virtual void printStatistics(FILE* file) = 0;

		/**
		 * Returns a string that will be appended into some intermediate
		 * statistics information of stage 1. Basically, the aligner should
		 * present how many steps have been calculated, giving an idea of
		 * conclusion percentage, and some quick information about pruning
		 * status. All the string should reside in a line (around 80
		 * characters).
		 *
		 * @return a single line progress strings without '\\n'.
		 */
		virtual const char* getProgressString() const = 0;

		/**
		 * Returns the number of cells that have been processed since the last
		 * call to clearStatistics.
		 *
		 * @return the number of processed cells.
		 */
		virtual long long getProcessedCells() = 0;



protected:
/* protected constructors avoid the direct creation/deletion of this interface */
		~IAligner() {};
		IAligner() {};


};

#endif /* IALIGNER_HPP_ */
