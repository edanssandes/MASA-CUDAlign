/*******************************************************************************
 *
 * Copyright (c) 2010-2015   Edans Sandes
 *
 * This file is part of MASA-CUDAlign.
 * 
 * MASA-CUDAlign is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * MASA-CUDAlign is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MASA-CUDAlign.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#ifndef SMITH_WATERMAN_H_
#define SMITH_WATERMAN_H_

#include "libmasa/libmasa.hpp"
#include "CUDAlignerParameters.hpp"

#include <vector_types.h>

/*
 * The CUDAligner class is the Aligner implementation of the MASA-CUDAlign
 * extensions. This class is divided in three files.
 *
 *  - CUDAlign.hpp: class header file. (compiled with g++)
 *  - CUDAlign.cpp: class implementation file. (compiled with g++)
 *  - CUDAlign.cu:  CUDA related code. (compiled with nvcc)
 *
 * Note that all these files must be compiled with the same number of threads,
 * i.e. the same -DTHREADS_COUNT parameter. Otherwise the execution
 * will not produce correct results.
 */



/**
 * Number of threads used in the CUDAlign kernels
 */
#ifndef THREADS_COUNT
#define THREADS_COUNT 64
#endif

/**
 * Maximum number of CUDA blocks in an external diagonal.
 */
#define MAX_BLOCKS_COUNT    512

/**
 * Number of rows processed by each thread. This constant must not be
 * changed, because there are many optimizations along the code that
 * uses exactly 4 rows (e.g., 4-pack parameters, loop unrolling, etc).
 */
#define ALPHA (4)

/**
 * The height of a CUDA grid, i.e. CUDA THREADS x CUDA BLOCKS.
 */
#define MAX_GRID_HEIGHT     (THREADS_COUNT*MAX_BLOCKS_COUNT)

/*
 * Alignment parameters. These constants may be changed to generate
 * a new executable with different Smith Waterman parameters.
 */

/**
 * Punctuation for a match (same letters). Must be positive.
 */
#define DNA_MATCH       (1)

/**
 * Penalty for a mismatch (different letters). Must be negative.
 */
#define DNA_MISMATCH    (-3)

/**
 * Penalty for opening a gap.
 */
#define DNA_GAP_OPEN    (3)

/**
 * Penalty for extending a gap. Must be positive.
 */
#define DNA_GAP_EXT     (2)

/**
 * PEnalty of the first gap. Must be positive. The first gap has a penalty
 * of -DNA_GAP_OPEN-DNA_GAP_EXT.
 */
#define DNA_GAP_FIRST   (DNA_GAP_EXT+DNA_GAP_OPEN)


/** @brief Structure containing all the host memory structures.
 * The host_structures_t and cuda_structures_t are used to transfer
 * data between CPU-GPU. Usually there are equivalent vectors in both
 * structures.
 */
typedef struct {
	/**
	 * Horizontal bus.
	 **/
	int2* h_busH;
	/**
	 * Vector for storing the last row shifted bus.
	 * @see CUDAligner::loadLastRow and kernel_flush methods
	 */
	int2* h_extraH;
	/**
	 * Vector that stores the H components of the last column. Each element
	 * of this vector is a 4-pack integer for the alpha rows.
	 */
	int4* h_flushColumnH;
	/**
	 * Vector that stores the E components of the last column. Each element
	 * of this vector is a 4-pack integer for the alpha rows.
	 */
	int4* h_flushColumnE;
	/**
	 * Vector used to interleave the 4-pack H and E components to a structure
	 * to be dispatched to the MASA-core.
	 * @see CUDAligner::getLastColumn() method.
	 */
	cell_t* h_flushColumn;
	/**
	 * Vector used to load the H components of the first column.
	 */
	int4* h_loadColumnH;
	/**
	 * Vector used to load the E components of the first column.
	 */
	int4* h_loadColumnE;

	/**
	 * Vector used to store the best score from each block.
	 */
	int4* h_blockResult;
	/**
	 * Vector used to store the best score from each block (score_t structure).
	 */
	score_t* h_blockScores;
} host_structures_t;

/** @brief Structure containing all the CUDA memory structures.
 */
typedef struct {
	/** CUDA vector containing the sequence S_0 (vertical) */
	unsigned char* d_seq0;
	/** CUDA vector containing the sequence S_1 (horizontal) */
	unsigned char* d_seq1;
	/** Equivalent vector of host_structures_t::h_busH */
	int2* d_busH;
	/** Equivalent vector of host_structures_t::h_extraH */
	int2* d_extraH;
	/** Equivalent vector of host_structures_t::h_flushColumnH */
	int4* d_flushColumnH;
	/** Equivalent vector of host_structures_t::h_flushColumnE */
	int4* d_flushColumnE;
	/** Equivalent vector of host_structures_t::h_loadColumnH */
	int4* d_loadColumnH;
	/** Equivalent vector of host_structures_t::h_loadColumnE */
	int4* d_loadColumnE;
	/** Equivalent vector of host_structures_t::h_blockResult */
	int4* d_blockResult;
	/** CUDA vector storing the H components of the vertical bus */
	int4* d_busV_h;
	/** CUDA vector storing the E components of the vertical bus */
	int4* d_busV_e;
	/** CUDA vector storing other components of the vertical bus */
	int3* d_busV_o;
	/** Size allocated for the d_busH vector */
	int busH_size;
} cuda_structures_t;



/** @brief IAligner implementation for the MASA-CUDAlign extension.
 *
 * This class is the Aligner implementation of the MASA-CUDAlign extension.
 * It extends from the AbstractAligner class.
 *
 * @see The AbstractAligner and IAligner to understand the
 * Aligner implementation.
 */
class CUDAligner : public AbstractDiagonalAligner {

public:
	/* Constructor and destructor */
	CUDAligner();
	~CUDAligner();

	/* Implementations of the IAligner virtual methods. @see IAligner class. */
	virtual aligner_capabilities_t getCapabilities();
	virtual AbstractAlignerParameters* getParameters();
	const virtual score_params_t* getScoreParameters();
	virtual void initialize();
	virtual void finalize();
	virtual void setSequences(const char* seq0, const char* seq1, int seq0_len, int seq1_len);
	virtual void unsetSequences();

	virtual void clearStatistics();
	virtual void printInitialStatistics(FILE* file);
	virtual void printStageStatistics(FILE* file);
	virtual void printFinalStatistics(FILE* file);
	virtual void printStatistics(FILE* file);



protected:
	virtual int getGridWidth(int width);
	virtual int getBlockHeight();

	/* methods that returns data from the accelerator */
	virtual cell_t* getSpecialRow(int j, int len);
	virtual cell_t* getLastRow(int j, int len);
	virtual cell_t* getLastColumn(int j, int len);
	virtual score_t* getBlockScores();

	virtual void setFirstRow(const cell_t* cells, int j, int len);
	virtual void setFirstColumn(const cell_t* cells, int i, int len);
	virtual void clearPrunedBlocks(int b0, int b1);

	virtual void initializeDiagonals();
	virtual void processDiagonal(int diagonal, int windowLeft, int windowRight);
	virtual void finalizeDiagonals();

private:
	/** Stores the host allocated memory */
	host_structures_t host;
	/** Stores the cuda allocated memory */
	cuda_structures_t cuda;

	/** Number of multiprocessor of the selected GPU */
	int multiprocessors;
	/** Command line parameters for this extension */
	CUDAlignerParameters* params;
	/** Smith Waterman parameters */
	score_params_t score_params;

	/*
	 * The following attributes are used for statistics purpose only.
	 */
	/** Total RAM memory in the GPU. */
	size_t statTotalMem;
	/** RAM memory used before initialization. */
	size_t statInitialUsedMemory;
	/** RAM memory allocated during initialization. */
	size_t statFixedAllocatedMemory;
	/** RAM memory allocated in the current stage. */
	size_t statVariableAllocatedMemory;
	/** RAM memory that was allocated but was not deallocated. */
	size_t statDeallocatedMemory;

	/***********************************************/
	/* See the method documentation on source file */
	/***********************************************/

	int getThreadCount();

	/* Memory allocation related methods */

	void allocateGlobalStructures();
	void freeGlobalStructures();

};


/*
 * The following methods are wrappers for the CUDAligner.cu source file.
 */

void lauch_external_diagonals(
		int COLUMN_SOURCE, int COLUMN_DESTINATION,
		int RECURRENCE_TYPE, int CHECK_LOCATION,
		const int blocks, const int threads,
		const int i0, const int i1,
		const int step, const int2 cutBlock, cuda_structures_t* cuda);

void bind_textures(const unsigned char* seq0, const int seq0_len,
		const unsigned char* seq1, const int seq1_len);
void unbind_textures();
void copy_split(const int* split, const int blocks);

void initializeBusHInfinity(const int p0, const int p1, int2* d_busH);


#endif /* SMITH_WATERMAN_H_ */
