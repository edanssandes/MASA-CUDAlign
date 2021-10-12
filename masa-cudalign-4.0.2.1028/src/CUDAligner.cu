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

#include "CUDAligner.hpp"
#include "cuda_util.h"
#include "config.h"

#include <stdio.h>
#include <getopt.h>

/**
 * @file CUDAligner.cu
 * @brief CUDAligner related kernels and wrappers.
 *
 * This file contains all the kernels and some wrappers for the
 * CUDAligner class. Ensure that all CUDAligner.* files are compiled
 * with the same number of threads.
 *
 * The function templates accelerate the computation using precompiled
 * different kernels. We have four templates: COLUMN_SOURCE can be 0 (first
 * column is zeroed) or 1 (first column is customized vector);
 * COLUMN_DESTINATION can be 0 (last
 * column is discarded) or 1 (last column is stored in a vector);
 * CHECK_LOCATION can be 0 (ignore best scores) or 1 (check for best scores);
 * and RECURRENCE_TYPE can be SMITH_WATERMAN or NEEDLEMAN_WUNSCH.
 *
 * @remark This file must be compiled by nvcc NVidia compiler.
 */


/** First column must be zeroes */
#define FROM_ZEROES			(0)
/** First column must be loaded with custom data from a vector. */
#define FROM_VECTOR			(1)


/** Discard last column cells */
#define DISCARD_LAST_COLUMN			(false)
/** Stores tha last column cells in a vector */
#define STORE_LAST_COLUMN			(true)


/** Check best score in the block cells. */
#define CHECK_BEST_SCORE			(true)
/** Do not check best score in the block cells. */
#define IGNORE_BEST_SCORE			(false)


/*
 * Texture definition for storing read-only large variables.
 */

/**
 * Texture definition for the DNA of sequence#0 (vertical in the DP matrix).
 */
texture<unsigned char, 1, cudaReadModeElementType> t_seq0;

/**
 * Texture definition for the DNA of sequence#1 (horizontal in the DP matrix).
 */
texture<unsigned char, 1, cudaReadModeElementType> t_seq1;


/**
 * Texture definition for the read-only part of the horizontal bus.
 * This is a tricky part, since the same horizontal bus is used for
 * writing and for reading. The data is read from the horizontal bus
 * in a linear pattern (left to right) using the texture. The data
 * is written in the same horizontal bus using the same linear pattern
 * (left to right), but the written data is shifted left some cells
 * compared with the reading data. So, the written data does not affect the
 * read-only data. In every external diagonal we clean the texture cache
 * for safety. For the single phase execution (small sequences), this
 * optimization must be disabled.
 */
texture<         int2, 1, cudaReadModeElementType> t_busH;



/*
 * Shared Memory structures, used for sharing data between the threads of
 * the same block.
 */

/**
 * Used to share the H data of the Smith-Waterman cell. We have a
 * double buffer structure to allow simultaneous non-block read-write.
 */
__shared__   int  s_colx [2][THREADS_COUNT]; // Could be THREADS_COUNT-1

/**
 * Used to share the F data of the Smith-Waterman cell. We have a
 * double buffer structure to allow simultaneous non-block read-write.
 */
__shared__   int  s_coly [2][THREADS_COUNT];

/**
 * Stores the column position that splits each block, e.g., block #0 processes
 * columns between d_split[0](inclusive) and d_split[1](exclusive). Generally,
 * block $i$ processes columns between d_split[i-1] and d_split[i].
 */
__constant__ int d_split[MAX_BLOCKS_COUNT+1];



/**
 * Returns true only to the thread with the maximum value. This function is
 * much faster than the findMaxSmall function, but it only works if the number
 * of threads is known in compilation time and if the number of threads is
 * a power of 2. So, do not use this function for generic number of threads.
 *
 * @param idx the thread Id.
 * @param val the value that will be compared
 * @return true to the thread with the maximum value. False otherwise. If
 *              more than one thread has the maximum value, only one of them
 *              will return true.
 */
__device__ int findMax(int idx, int val) {
    __shared__ int s_max[THREADS_COUNT];
    __shared__ int s_idx;
    int count = THREADS_COUNT>>1;
    s_max[idx] = val;
    __syncthreads();

    while (count > 0) {
        if (idx < count) {
            if (s_max[idx] < s_max[idx+count]) {
                s_max[idx] = s_max[idx+count];
            }
        }
        count = count >> 1;
        __syncthreads();
    }
    if (s_max[0] == val) {
        s_idx = idx;
    }
    __syncthreads();
    return (s_idx == idx);
}

/**
 * Returns true only to the thread with the maximum value. This function is
 * slower then the findMax function, but it works for a variable number of
 * threads. This should be used in the kernel with a single block, since
 * the number of threads cannot be greater than the size of sequence #2
 * (horizontal sequence).
 *
 * @param idx the thread Id.
 * @param val the value that will be compared
 * @return true to the thread with the maximum value. False otherwise. If
 *              more than one thread has the maximum value, only one of them
 *              will return true.
 */
__device__ int findMaxSmall(int idx, int val) {
	__shared__ int s_max;
	__shared__ int s_idx;

	s_max = -INF;
	__syncthreads();

	atomicMax(&s_max, val);
	if (s_max == val) {
		s_idx = idx;
	}
    __syncthreads();
    return (s_idx == idx);
}

/**
 * Consider that each thread processes ALPHA rows, so the last row of the
 * matrix may be any of the ALPHA rows of the last thread. Sometimes, there
 * will be some rows of the thread that will not be processed. Furthermore,
 * the last row of the DP matrix must be flushed to the disk in many situations,
 * so, the very last thread of a block must save all its data in different
 * structures.
 *
 * This function returns information about which rows of the thread must
 * be processed, considering that the DP matrix must be fully processed
 * between rows $i0$ and $i1$.
 *
 * @param i the number of the first row (top) of the thread.
 * @param i0 the number of the first row (top) of the processed area
 *           of the DP matrix (usually 0 in stage 1).
 * @param i1 the number of the last row (bottom) of the processed area
 *           of the DP matrix (usually the length of seq0 in stage 1).
 *
 * @return Possible return values are:
 *   0: if all the rows of the thread is out of range [i0..i1].
 *   -1: if all the rows of the thread is in the range [i0..i1].
 *   1: if only the first row [1] of the thread is in the range [i0..i1].
 *   2: if only the rows [1,2] of the thread are in the range [i0..i1].
 *   3: if only the rows [1,2,3] of the thread are in the range [i0..i1].
 */
static __device__ int get_flush_id(const int i, const int i0, const int i1) {
    if (i < i0 || i >= i1) {
    	return 0;
    } else {
    	return (i+ALPHA < i1) ? -1 : ((i1-i0-1)%ALPHA)+1;
    }
}

/**
 * Returns the maximum of two numbers.
 */
static __device__ int my_max(int a, int b) {
    return (a>b)?a:b;
}

/**
 * Returns the maximum of three numbers.
 */
static __device__ int my_max3(int a, int b, int c) {
    return my_max(a, my_max(b,c));
}

/**
 * Returns the maximum of four numbers.
 */
static __device__ int my_max4(int a, int b, int c, int d) {
    return my_max(my_max(a,b), my_max(c,d));
}

/**
 * Fetch a block with 4 nucleotides at positions [i..i+3] of sequence t_seq0.
 *
 * @param i the row of the first base of the DNA 4-block.
 *        The i index is 0-based.
 * @return ss the 4 bases in the (x,y,z,w) uchar4 vector, representing
 *         the bases (i, i+1, i+2, i+3) of the sequence t_seq0.
 */
static __device__ void fetchSeq0(const int i, uchar4* ss) {
	if (i >= 0) {
        ss->x = tex1Dfetch(t_seq0,i);
        ss->y = tex1Dfetch(t_seq0,i+1);
        ss->z = tex1Dfetch(t_seq0,i+2);
        ss->w = tex1Dfetch(t_seq0,i+3);
	}
}


/**
 * This procedure computes the cell (i,j) using the Smith-Waterman recurrence
 * formula or the Needleman-Wunsch recurrence formula. Consider that the
 * we use the affine-gap model, so the Dynamic Programming (DP) matrix is
 * compose of cells with 3 components (H, E, F). See the Gotoh algorithm
 * for better comprehension.
 *
 * @tparam RECURRENCE_TYPE	Template for SMITH_WATERMAN or NEEDLEMAN_WUNSCH.
 * @param[in] s0	The variable containing seq0[i] nucleotide.
 * @param[in] s1	The variable containing seq1[i] nucleotide.
 * @param[in,out] e00	Input: value of E(i,j-1). Output: value of E(i,j)
 * @param[in,out] f00	Input: value of F(i,j-1). Output: value of F(i,j)
 * @param[in] h01	value of H(i,j-1)
 * @param[in] h11	value of H(i-1,j-1)
 * @param[in] h10	value of H(i-1,j)
 * @param[out] h00  value of H(i,j)
 */
template <int RECURRENCE_TYPE>
__device__ void kernel_sw(const unsigned char s0, const unsigned char s1,
						  int *e00, int *f00, const int h01, const int h11, const int h10, int *h00) {

    *e00 = my_max(h01-DNA_GAP_OPEN, *e00)-DNA_GAP_EXT; // Horizontal propagation
    *f00 = my_max(h10-DNA_GAP_OPEN, *f00)-DNA_GAP_EXT; // Vertical propagation
    int v1 = h11+((s1!=s0)?DNA_MISMATCH:DNA_MATCH);

    if (RECURRENCE_TYPE == SMITH_WATERMAN) {
    	*h00 = my_max4(0, v1, *e00, *f00);
    } else if (RECURRENCE_TYPE == NEEDLEMAN_WUNSCH) {
    	*h00 = my_max3(v1, *e00, *f00);
    }
}

/**
 * Updates the maximum score found ($max$) and its position ($pos$) after
 * the computation of the cell at position (i+inc,j). The $i+inc$ coordinate
 * considers that each thread computes $ALPHA$ cells, so we split the
 * coordinate in $i$ and $inc$, where $i$ is the first coordinate of the ALPHA
 * cells and $inc$ increases the coordinate of the following cells. When the
 * $absolute_row$ parameter is true, the best score position ($pos$) is
 * updated with the absolute coordinate $i+inc$, but if $absolute_row$ is false,
 * then $pos$ is updated with the incremental $inc$ value, avoiding one add
 * operation. In this case, the missing add operation is done only once
 * in the end of the kernel execution, leading to better performance.
 *
 * @param[in] i			The index of the row.
 * @param[in] j			The index of the column.
 * @param[in,out] max 	Input: previous maximum score.
 * 						Output: updated maximum score.
 * @param[in,out] pos 	Input: coordinates of the previous maximum score.
 * 								See the $absolute_row$ parameter.
 *            			Output: updated coordinates of the maximum score.
 * @param[in] h00 	The score of cell (i+inc,j). Only the H component is necessary,
 *                   	since the other components E and F cannot be greater than H.
 * @param[in] inc 	This is the row id inside each thread neighborhood.
 * @param[in] absolute_row 	If it is true, the $pos$ parameter stores the absolute
 *                   coordinates of the cell (i+inc, j). Otherwise, if
 *                   $store$ is false, the $pos$ parameters stores the relative
 *                   coordinates of the cell (inc, j).
 */
__device__ void kernel_check_max(const int i, const int j, int *max,
		int2 *pos, const int h00, const int inc, const bool absolute_row) {
	if (*max < h00) {
		*max = h00;
		pos->x = j;
		if (absolute_row) {
			pos->y = i+inc;
		} else {
			pos->y = inc;
		}
	}
}

/**
 * Computes a block of ALPHA cells inside the same thread (K-neighborhood).
 * The computation is done serially (up-down). The processed cells are
 * (i,j), (i+1,j), (i+2,j), (i+3,j), considering that ALPHA is 4.
 *
 * @tparam RECURRENCE_TYPE	Defines with recurrence function will be use. It
 * 								can be SMITH_WATERMAN (local alignment) or
 * 								NEEDLEMAN_WUNSCH (global alignment).
 * @tparam FLUSH_LAST_ROW	Defines if the flush_id parameter will be used.
 * 								If it is false,	all the ALPHA rows will be
 * 								processed regardless if they are out-of-range.
 * 								Otherwise, only valid rows will be processed.
 *
 * @param[in] i		The index of the row (top cell).
 * @param[in] j		The index of the column.
 * @param[in] s1	One nucleotide related to sequence#1 (horizontal).
 * @param[in] ss	A block with 4 (ALPHA) nucleotides related to
 * 						sequence#0 (vertical).
 * @param[in,out] left_e	Input: Values of E(i,j-1)...E(i+3,j-1)
 * 							Output: Values of E(i,j)...E(i+3,j)
 * @param[in,out] up_f		Input: Value of F(i-1,j)
 * 							Output: Value of F(i+3,j)
 * @param[in] left_h		Input: Value of H(i,j-1)...H(i+3,j-1)
 * @param[in] diag_h		Input: Value of H(i-1,j-1)
 * @param[in] up_h			Input: Value of H(i-1,j)
 * @param[out] curr_h		Output: Value of H(i,j)...H(i+3,j)
 * @param[in] flush_id		Identifier of the last row. This value controls with
 * 								row will be computed in the bottom-most blocks.
 * 								See get_flush_id function.
 *
 */
template <int RECURRENCE_TYPE, bool FLUSH_LAST_ROW>
__device__ void kernel_sw4(const int i, const int j, const unsigned char s1, const uchar4 ss,
									int4 *left_e, int *up_f, const int4 left_h, const int diag_h, const int up_h, int4 *curr_h, const int flush_id) {
	kernel_sw<RECURRENCE_TYPE>(s1, ss.x, &left_e->x, up_f, left_h.x, diag_h  , up_h  , &curr_h->x);
	if (FLUSH_LAST_ROW && flush_id == 1) return;
	kernel_sw<RECURRENCE_TYPE>(s1, ss.y, &left_e->y, up_f, left_h.y, left_h.x, curr_h->x, &curr_h->y);
	if (FLUSH_LAST_ROW && flush_id == 2) return;
	kernel_sw<RECURRENCE_TYPE>(s1, ss.z, &left_e->z, up_f, left_h.z, left_h.y, curr_h->y, &curr_h->z);
	if (FLUSH_LAST_ROW && flush_id == 3) return;
	kernel_sw<RECURRENCE_TYPE>(s1, ss.w, &left_e->w, up_f, left_h.w, left_h.z, curr_h->z, &curr_h->w);
}

/**
 * Updates the maximum score of a block of ALPHA cells (K-neighborhood).
 *
 * @tparam CHECK_MAX_SCORE	Defines if the function will be executed. If it is false,
 * 								this function does absolutely nothing.
 * @tparam FLUSH_LAST_ROW	Defines if the flush_id parameter will be used.
 * 								If it is false,	all the ALPHA rows will be
 * 								processed regardless if they are out-of-range.
 * 								Otherwise, only valid rows will be processed.
 *
 * @param[in] i		The index of the row (top cell).
 * @param[in] j		The index of the column.
 * @param[in,out] max		The maximum score to be updated.
 * @param[in,out] max_pos	The coordinates of the maximum score.
 * @param[in] curr_h		The values of H(i,j)...H(i+3,j)
 * @param[in] absolute_row 	Defines if the row coordinates (i) stored in the max_pos
 * 							parameter must be absolute or relative.
 * 							If it is relative (false), the coordinates row may only be in
 * 							the range 0..3. If it is absolute (true), the coordinates
 * 							row is $i+inc$.
 * @param[in] flush_id		Identifier of the last row. See get_flush_id function.
 */
template <bool CHECK_MAX_SCORE, bool FLUSH_LAST_ROW>
__device__ void kernel_check_max4(const int i, const int j,
						  int *max, int2 *max_pos,
						  int4 *curr_h,
						  const bool absolute_row, const int flush_id) {
	if (CHECK_MAX_SCORE) {
		kernel_check_max(i, j, max, max_pos, curr_h->x, 0, absolute_row);
		if (FLUSH_LAST_ROW && flush_id == 1) return;
		kernel_check_max(i, j, max, max_pos, curr_h->y, 1, absolute_row);
		if (FLUSH_LAST_ROW && flush_id == 2) return;
		kernel_check_max(i, j, max, max_pos, curr_h->z, 2, absolute_row);
		if (FLUSH_LAST_ROW && flush_id == 3) return;
		kernel_check_max(i, j, max, max_pos, curr_h->w, 3, absolute_row);
	}
}

/**
 * This function loads the dependencies H(i-1,j) and F(i-1,j) before computing
 * cells (i,j)..(i,j+3). Furthermore, the nucleotide of column $j$ is read
 * from sequence#1 (horizontal).
 *
 * The values H(i-1,j) and F(i-1,j) may be read from
 * the shared memory or from the horizontal bus, depending if $i$ is the
 * first row of the block or not. If USE_TEXTURE_CACHE is used, the
 * horizontal bus is read from a texture cache (t_busH). See the t_busH comments
 * for a better comprehension of this optimization and its pitfall.
 * For very small sequences, this optimization does not work and the
 * horizontal bus is read directly from the busH global vector.
 *
 * @tparam USE_TEXTURE_CACHE	Enables or disable the t_busH texture read. If
 * 									it is disabled, the read is made directly
 * 									from the original busH global vector.
 *
 * @param[in] idx	Index of the current thread.
 * @param[in] bank		The shared memory bank (1 or 0) from which the data will be taken.
 *						This is necessary for the double buffer used in the shared memory.
 *						If bank is 0, the data is read from bank 0 and written in bank 1.
 *						If bank is 1, the data is read from bank 1 and written in bank 0.
 * @param[in] j		The index of the column.
 * @param[in] busH		Horizontal bus. This is the global vector from which we
 * 						load the H(i-1,j) and F(i-1,j) values for the first thread.
 * @param[out] h	The loaded value of H(i-1,j)
 * @param[out] f	The loaded value of F(i-1,j)
 * @param[out] s	The j-th dna nucleotide of sequence#1 (horizontal)
 */
template <bool USE_TEXTURE_CACHE>
__device__ void kernel_load(const int idx, const int bank, const int j, int2* busH, int *h, int *f, unsigned char *s) {
	*s = tex1Dfetch(t_seq1, j);
    if (idx) {
    	// Threads (except the first one) must read from the shared memory.
        *h = s_colx[bank][idx];
        *f = s_coly[bank][idx];
    } else {
    	// First thread of the block must read from the horizontal bus.
        int2 temp;
        if (USE_TEXTURE_CACHE) {
        	temp = tex1Dfetch(t_busH,j); // read from texture
        } else {
        	temp = busH[j]; // read directly from the busH global vector
        }
        *h = temp.x; // H-component
        *f = temp.y; // F-component
    }
}


/**
 * This function writes the values H(i+3,j) and F(i+3,j) of the thread into the
 * horizontal bus or into the shared memory. With this procedure, the next
 * thread can load its dependencies in the next internal diagonal. See function
 * kernel_load.
 *
 * If the thread has the last row of the matrix, this row may be saved in
 * the busH/extraH vectors for further usage. Otherwise, the busH structure
 * may contain values of an out-of-range row (since it may not be multiple of 4
 * and it may be unaligned with the last row of the block).
 *
 * @tparam  FLUSH_LAST_ROW		Indicates if the last row of the matrix must be
 * 									stored in the busH/extraH vectors
 *
 * @param[in] i		The index of the row (top cell).
 * @param[in] j		The index of the column.
 * @param[in] idx	Index of the current thread.
 * @param[in] bank	The shared memory bank (1 or 0) into which the data will be stored.
 *						This is necessary for the double buffer used in the shared memory.
 *						If bank is 1, the data is read from bank 0 and written in bank 1.
 *						If bank is 0, the data is read from bank 1 and written in bank 0.
 * @param[out] busH		Horizontal bus. This is the global vector that receives the
 * 						H(i+3,j) and F(i+3,j) values. If the FLUSH_LAST_ROW is active,
 * 						the busH will receive the H(m,j) and F(m,j) values, where
 * 						$m$ is the last row of the whole DP matrix. Furthermore,
 * 						when the FLUSH_LAST_ROW is active, the H(m,j) and F(m,j)
 * 						values will be stored in the busH[m-THREAD_COUNT] cell,
 * 						with THREAD_COUNT cells shifted to the left.
 * @param[out] extraH	Extra horizontal bus used if the FLUSH_LAST_ROW is active.
 * 						This is an auxiliary structure that receives
 * 						the values of the horizontal bus that may have an negative
 * 						index (since it is shifted THREAD_COUNT cells to the left).
 * @param[in] h 		The values of H(i,j)..H(i+3,j). This is only used when
 * 						FLUSH_LAST_ROW is active, since we must flush only
 * 						the last row of the DP matrix. Otherwise, only the
 * 						H(i+3,j) will be flushed.
 * @param[in] f			The value of F(i+3,j).
 * @param[in] last_thread	The index of the last thread of the block.
 * @param[in] flush_id		The index of the last DP row in the thread. See function get_flush_id.
 */
template <bool FLUSH_LAST_ROW>
__device__ void kernel_flush(const int i, const int j, const int idx, const int bank, int2* busH, int2* extraH, const int4* h, const int f, const int last_thread, const int flush_id) {
	if (FLUSH_LAST_ROW && flush_id > 0) {
		int h00;
		if (flush_id == 1) {
			h00 = h->x;
		} else if (flush_id == 2) {
			h00 = h->y;
		} else if (flush_id == 3) {
			h00 = h->z;
		} else if (flush_id == 4) {
			h00 = h->w;
		}

		/*
		 * The last row must be shifted THREADS_COUNT bytes to the left in
		 * order to prevent the overwrite of the previous special rows (i.e
		 * since the busH structure is shared by all the blocks, the
		 * bottom-most blocks may overwrite the result of the other blocks).
		 * Since the busH may not be written in negative index,
		 * the extraH vector is used to store this out-of-bound cells.
		 * The remaining cells are written directly to the busH.
		 */
		int2 temp = make_int2(h00, f);
		int adj = j-THREADS_COUNT;
		if (adj < d_split[0]) {
			extraH[j-d_split[0]] = temp;
		} else {
			busH[adj] = temp;
		}
	} else if (idx == last_thread) {
		int2 temp = make_int2(h->w, f);
        busH[j] = temp; // Store into the busH global vector.
    } else {
    	// Store into the shared memory.
        s_colx[bank][idx+1] = h->w;
        s_coly[bank][idx+1] = f;
    }
}


/**
 * After incrementing the column j, this procedure must be called to check if $j$
 * overflows the edge of the sequence#1 (horizontal). If this happens, we must
 * set $j$ to zero and continue the computation in the proper line. In this situation,
 * all registers must be reinitialized in order to represent the first column
 * of the matrix. If the COLUMN_DESTINATION is set with TO_VECTOR, the last
 * column is stored in memory.
 *
 * The in/out params will only be updated if the overflow occurs. Note that
 * every time an overflow happens, the updated cells will consider the kind
 * of alignment (global/semi-global/local) and the source/destination of the values
 * in the edges of the matrix (vector, gap patter or zeroes). See the COLUMN_SOURCE
 * templated variable.
 *
 * @tparam COLUMN_SOURCE 	Indicates how to load the first column. Possible values are:
 * 							FROM_NULL: First column is all zeroed (local alignment)
 * 							FROM_VECTOR: First column is loaded from the loadColumn_h/loadColumn_e vectors.
 * @tparam COLUMN_DESTINATION 	Indicates how to save the last column. Possible values are:
 * 							TO_NULL: Last column is ignored
 * 							TO_VECTOR: Last column is saved into flushColumn_h/flushColumn_e vectors.
 *
 * @param[in] i0	the first row of the DP matrix
 * @param[in] j0	the first column of the DP matrix
 * @param[in] i1	the last row of the DP matrix
 * @param[in] j1	the last column of the DP matrix
 * @param[in,out] i		Input: the current row.
 * 						Output: the updated row if the overflow occurred.
 * @param[in,out] j		Input: the current column.
 * 						Output: the updated column if the overflow occurred.
 * @param[in,out] ss    Input: the variable containing 4 nucleotides of sequence#0.
 * 						Output: the next 4 nucleotides if the overflow occurred.
 * @param[in,out] ee 	Input: the values E(i,j)...E(i+3,j)
 * 						Output: the values E(i,0)...E(i+3,0) if the overflow occurred.
 * @param[in,out] h10 	Input: the values H(i-1,j)
 * 						Output: the values H(i-1,0) if the overflow occurred.
 * @param[in,out] h00 	Input: the values H(i,j)...H(i+3,j)
 * 						Output: the values H(i,0)...H(i+3,0) if the overflow occurred.
 * @param[in,out] flush_id	Input: The index of the last DP row in the thread. See function get_flush_id.
 * 							Output: the updated flush_id if the overflow occurred.
 * @param[in] 	loadColumn_h,loadColumn_e	the values of the first column if COLUMN_SOURCE==FROM_VECTOR.
 * @param[out]  flushColumn_h,flushColumn_e	the destination of the last column if COLUMN_DESTINATION==TO_VECTOR.
 * @param[in]   idx			Index of the current thread.
 * @param[in] 	HEIGHT 		how many lines must be jumped if the overflow occurs.
 */
template <int COLUMN_SOURCE, int COLUMN_DESTINATION>
__device__ void kernel_check_bound(const int i0, const int j0, const int i1, const int j1, int *i, int *j,
        uchar4* ss, int4 *ee, int *h10,  int4 *h00, int *flush_id,
		const int4* loadColumn_h, const int4* loadColumn_e,
		int4* flushColumn_h, int4* flushColumn_e,
		const int idx, const int HEIGHT) {
    if (*j>=d_split[gridDim.x]) {
		if (COLUMN_DESTINATION == STORE_LAST_COLUMN) {
            flushColumn_h[idx] = *h00;
            flushColumn_e[idx] = *ee;
		}

		*j=d_split[0];
        *i+=HEIGHT;

        *flush_id = get_flush_id(*i, i0, i1);

        if (COLUMN_SOURCE == FROM_ZEROES) {
			*ee=make_int4(-INF,-INF,-INF,-INF);
			*h00=make_int4(0,0,0,0);
			*h10=0;
		} else if (COLUMN_SOURCE == FROM_VECTOR) {
			*h10 = loadColumn_h[idx].w;
			*h00 = loadColumn_h[idx+1];
			*ee = loadColumn_e[idx+1];
		}
		fetchSeq0(*i, ss);
    }
}


/**
 * This function processes all the THREADS_COUNT-1 internal diagonals of the short phase.
 *
 * @tparam COLUMN_SOURCE, COLUMN_DESTINATION, RECURRENCE_TYPE, CHECK_LOCATION
 * 				See these templates in the "Detailed Description" section in the beginning of this file.
 *
 * @param[in] idx	Index of the current thread in the block.
 * @param[in] tidx	Index of the current thread in the grid.
 * @param[in] i		The index of the row (top cell).
 * @param[in] j		The index of the column.
 * @param[in] i0,i1	the first and last row of the DP matrix
 * @param[in] j0,j1	the first and last column of the DP matrix
 * @param[in,out] busH		Horizontal bus used to transfer data between blocks (top-down).
 * @param[out] extraH		Extra Horizontal bus. See kernel_flush function for more information.
 * @param[in,out] busV_h,busV_e,busV_o		Vertical bus used to transfer data between blocks (left-right).
 * @param[in,out] max,max_pos	Best score and its position.
 * @param[in] flush_id	The index of the last DP row in the thread. See function get_flush_id.
 * @param[in] loadColumn_h,loadColumn_e		The values of the first column (when COLUMN_SOURCE=FROM_VECTOR).
 * @param[out] flushColumn_h,flushColumn_e		Stores the last column (when COLUMN_DESTINATION=TO_VECTOR).
 */
template<int COLUMN_SOURCE, int COLUMN_DESTINATION, int RECURRENCE_TYPE, int CHECK_LOCATION, bool FLUSH_LAST_ROW>
__device__ void process_internal_diagonals_short_phase(
		const int idx, const int tidx, int i, int j,
		const int i0, const int j0, const int i1, const int j1,
		int2* busH, int2* extraH,
		int4* busV_h, int4* busV_e, int3* busV_o,
		int* max, int2* max_pos,
		int flush_id,
		const int4* loadColumn_h, const int4* loadColumn_e,
		int4* flushColumn_h, int4* flushColumn_e)
{
    s_colx[0][idx] = s_colx[1][idx] = busV_o[tidx].x; // TODO poderia ser pego de busV_h[tidx].w. Certo?
    s_coly[0][idx] = s_coly[1][idx] = busV_o[tidx].y;

    int4 left_h = busV_h[tidx];
	int4 left_e = busV_e[tidx];
    int  diag_h = busV_o[tidx].z;

    uchar4 ss;
	fetchSeq0(i, &ss);

    __syncthreads(); // barrier


	/*
	 *  We need THREADS_COUNT-1 Steps to complete the pending cells.
	 */

	int _k = (THREADS_COUNT>>1)-1; // we divide per 2 because we are loop-unrolling. THREADS_COUNT must be even.
	for (; _k; _k--) {
		int4 curr_h;
		int up_h;
		int up_f;

		/* Loop-unrolling #1 */
		kernel_check_bound<COLUMN_SOURCE, COLUMN_DESTINATION>(i0, d_split[0], i1, d_split[gridDim.x], &i, &j, &ss, &left_e, &diag_h, &left_h, &flush_id, loadColumn_h, loadColumn_e, flushColumn_h, flushColumn_e, idx, blockDim.x*gridDim.x*ALPHA);
		if (flush_id) {
			unsigned char s1;
			kernel_load<true>(idx, 1, j, busH, &up_h, &up_f, &s1);
			kernel_sw4<RECURRENCE_TYPE, FLUSH_LAST_ROW>(i, j, s1, ss, &left_e, &up_f, left_h, diag_h , up_h , &curr_h, flush_id);
			kernel_check_max4<CHECK_LOCATION, FLUSH_LAST_ROW>(i, j, max, max_pos, &curr_h, true, flush_id);
			kernel_flush<FLUSH_LAST_ROW>(i, j, idx, 0, busH, extraH, &curr_h, up_f, THREADS_COUNT-1, flush_id);
		}
		j++;
		__syncthreads(); // barrier

		/* Loop-unrolling #2 */
		kernel_check_bound<COLUMN_SOURCE, COLUMN_DESTINATION>(i0, d_split[0], i1, d_split[gridDim.x], &i, &j, &ss, &left_e, &up_h, &curr_h, &flush_id, loadColumn_h, loadColumn_e, flushColumn_h, flushColumn_e, idx, blockDim.x*gridDim.x*ALPHA);
		if (flush_id) {
			unsigned char s1;
			kernel_load<true>(idx, 0, j, busH, &diag_h, &up_f, &s1);
			kernel_sw4<RECURRENCE_TYPE, FLUSH_LAST_ROW>(i, j, s1, ss, &left_e, &up_f, curr_h, up_h , diag_h, &left_h, flush_id);
			kernel_check_max4<CHECK_LOCATION, FLUSH_LAST_ROW>(i, j, max, max_pos, &left_h, true, flush_id);
			kernel_flush<FLUSH_LAST_ROW>(i, j, idx, 1, busH, extraH, &left_h, up_f, THREADS_COUNT-1, flush_id);
		}
		j++;
		__syncthreads(); // barrier
	}

	{
		/* Last iteration is odd, so we put it outside the unrolled loop */
		int4 curr_h;
		int up_h;
		int up_f;
		kernel_check_bound<COLUMN_SOURCE, COLUMN_DESTINATION>(i0, j0, i1, j1, &i, &j, &ss, &left_e, &diag_h, &left_h, &flush_id, loadColumn_h, loadColumn_e, flushColumn_h, flushColumn_e, idx, blockDim.x*gridDim.x*ALPHA);
		if (flush_id) {
			unsigned char s1;
			kernel_load<true>(idx, 1, j, busH, &up_h, &up_f, &s1);
			kernel_sw4<RECURRENCE_TYPE, FLUSH_LAST_ROW>(i, j, s1, ss, &left_e, &up_f, left_h, diag_h, up_h, &curr_h, flush_id);
			kernel_check_max4<CHECK_LOCATION, FLUSH_LAST_ROW>(i, j, max, max_pos, &curr_h, true, flush_id);
			kernel_flush<FLUSH_LAST_ROW>(i, j, idx, 0, busH, extraH, &curr_h, up_f, THREADS_COUNT-1, flush_id);
		}
		j++;
		__syncthreads(); // barrier

		/* We store the result in the vertical bus to be read by the next block */

		kernel_check_bound<COLUMN_SOURCE, COLUMN_DESTINATION>(i0, j0, i1, j1, &i, &j, &ss, &left_e, &up_h, &curr_h, &flush_id, loadColumn_h, loadColumn_e, flushColumn_h, flushColumn_e, idx, blockDim.x*gridDim.x*ALPHA);
		busV_h[tidx] = curr_h;
		busV_e[tidx] = left_e;
		busV_o[tidx].x = s_colx[0][idx];
		busV_o[tidx].y = s_coly[0][idx];
		busV_o[tidx].z = up_h;
	}
}


/**
 * This kernel processes all the internal diagonal during the short phase. The
 * short phase processes the first THREAD_COUNT-1 diagonals and the long phase
 * processes the remaining diagonals. Each internal diagonal are processed
 * by many threads in parallel, but with a synchronous barrier after each
 * internal diagonal.
 *
 * @tparam COLUMN_SOURCE, COLUMN_DESTINATION, RECURRENCE_TYPE, CHECK_LOCATION
 * 				See these templates in the "Detailed Description" section in the beginning of this file.
 *
 * @param[in] i0,i1		the first and last row of the DP matrix
 * @param[in] step		the id of the external diagonal (0-based)
 * @param[in] cutBlock	(cutBlock.x, cutBlock.y) is the block pruning window.
 * @param[in,out] blockResult 	stores the best score and its position for each block.
 * @param[in,out] busH		Horizontal bus used to transfer data between blocks (top-down).
 * @param[out] extraH		Extra Horizontal bus. See kernel_flush function for more information.
 * @param[in,out] busV_h,busV_e,busV_o		Vertical bus used to transfer data between blocks (left-right).
 * @param[in] loadColumn_h,loadColumn_e		The values of the first column (when COLUMN_SOURCE=FROM_VECTOR).
 * @param[out] flushColumn_h,flushColumn_e		Stores the last column (when COLUMN_DESTINATION=TO_VECTOR).
 */
template <int COLUMN_SOURCE, int COLUMN_DESTINATION, int RECURRENCE_TYPE, bool CHECK_MAX_SCORE>
//__launch_bounds__(THREADS_COUNT,MIN_BLOCKS_PER_SM)
__global__ void kernel_short_phase(const int i0, const int i1,
				const int step, const int2 cutBlock, int4 *blockResult,
				int2* busH, int2* extraH,
				int4* busV_h, int4* busV_e, int3* busV_o,
				const int4* loadColumn_h, const int4* loadColumn_e,
				int4* flushColumn_h, int4* flushColumn_e)
{
    int bx = blockIdx.x;
    int by = step-bx;
    if (by < 0) return;

    int idx = threadIdx.x;

    const volatile int x0 = d_split[bx];
    const int xLen = d_split[bx+1] - x0;

    int i=(by*THREADS_COUNT)+idx;
    int tidx = (i % (blockDim.x*gridDim.x));

    // Block Pruning
    bool pruneBlock;
    if (bx != 0) {
    	pruneBlock = (bx < cutBlock.x || bx > cutBlock.y);
    } else {
    	pruneBlock = (cutBlock.x > 0 && cutBlock.y < blockIdx.x);
    }
	if (pruneBlock) {
		return;
	}

	const int j0 = d_split[0];
	const int j1 = d_split[gridDim.x];

    int j=x0-idx;
    i *= ALPHA;
    i += i0;
    if (j <= j0) {
        j += (j1 - j0);
        i -= (blockDim.x*gridDim.x)*ALPHA;
    }

    int2 max_pos;
    max_pos.x = blockResult[blockIdx.x].x;
    max_pos.y = blockResult[blockIdx.x].y;
    int max = blockResult[blockIdx.x].w;

	//int flush_id = get_flush_id(i, i0, i1);

        if (i >= i1) return;

	if (i < i1) {
		int block_i = (by*THREADS_COUNT)*ALPHA + i0;
		const int flush_id = get_flush_id(i, i0, i1); // TODO why this line is not inside the else clause (see long phase)? Check this.
		if (block_i+THREADS_COUNT*ALPHA < i1) {
			// If the block is fully inside the range [i0..i1], then we do not request to flush the last row.
			process_internal_diagonals_short_phase<COLUMN_SOURCE, COLUMN_DESTINATION, RECURRENCE_TYPE, CHECK_MAX_SCORE, false>(idx, tidx, i, j, i0, j0, i1, j1, busH, extraH, busV_h, busV_e, busV_o, &max, &max_pos, flush_id, loadColumn_h, loadColumn_e, flushColumn_h, flushColumn_e);
		} else {
			// Otherwise, we must flush the last row
			process_internal_diagonals_short_phase<COLUMN_SOURCE, COLUMN_DESTINATION, RECURRENCE_TYPE, CHECK_MAX_SCORE, true> (idx, tidx, i, j, i0, j0, i1, j1, busH, extraH, busV_h, busV_e, busV_o, &max, &max_pos, flush_id, loadColumn_h, loadColumn_e, flushColumn_h, flushColumn_e);
		}
    }


	/* Updates the block result with the block best score */

	if (CHECK_MAX_SCORE) {
		if (findMax(idx, max)) {
			blockResult[blockIdx.x].w = max;
			blockResult[blockIdx.x].x = max_pos.x;
			blockResult[blockIdx.x].y = max_pos.y;
			if (blockResult[blockIdx.x].z < max) {
				blockResult[blockIdx.x].z = max;
			}
		}
	} else {
    	blockResult[blockIdx.x].w = -INF;
	}
}

/**
 * This function processes all the internal diagonals of the long phase.
 *
 * @tparam RECURRENCE_TYPE, CHECK_LOCATION, FLUSH_LAST_ROW
 * 				See these templates in the "Detailed Description" section in the beginning of this file.
 *
 * @param[in] xLen	Number of internal diagonals.
 * @param[in] idx	Index of the current thread in the block.
 * @param[in] tidx	Index of the current thread in the grid.
 * @param[in] i		The index of the row (top cell).
 * @param[in] j		The index of the column.
 * @param[in,out] busH		Horizontal bus used to transfer data between blocks (top-down).
 * @param[out] extraH		Extra Horizontal bus. See kernel_flush function for more information.
 * @param[in,out] busV_h,busV_e,busV_o		Vertical bus used to transfer data between blocks (left-right).
 * @param[in,out] max,max_pos	Best score and its position.
 * @param[in] flush_id	The index of the last DP row in the thread. See function get_flush_id.
 */
template<int RECURRENCE_TYPE, int CHECK_LOCATION, bool FLUSH_LAST_ROW>
__device__ void process_internal_diagonals_long_phase(const int xLen, const int idx, const int tidx,
		const int i, int j,
		int2* busH, int2* extraH,
		int4* busV_h, int4* busV_e, int3* busV_o,
		int* max, int2* max_pos,
		const int flush_id)
{

    s_colx[0][idx] = s_colx[1][idx] = busV_o[tidx].x;
    s_coly[0][idx] = s_coly[1][idx] = busV_o[tidx].y;

    int4 left_h = busV_h[tidx];
	int4 left_e = busV_e[tidx];
    int  diag_h = busV_o[tidx].z;

    uchar4 ss;
	fetchSeq0(i, &ss);

    __syncthreads();


	int _k = xLen;
	if (_k&1) { // if odd
		int4 cur_h;
		int up_h;
		int up_f;

		unsigned char s1;
		kernel_load<true>(idx, 1, j, busH, &up_h, &up_f, &s1);
		kernel_sw4<RECURRENCE_TYPE, FLUSH_LAST_ROW>(i, j, s1, ss, &left_e, &up_f, left_h, diag_h, up_h, &cur_h, flush_id);
		kernel_check_max4<CHECK_LOCATION, FLUSH_LAST_ROW>(i, j, max, max_pos, &cur_h, false, flush_id);
		kernel_flush<FLUSH_LAST_ROW>(i, j, idx, 0, busH, extraH, &cur_h, up_f, THREADS_COUNT-1, flush_id);
		j++;
		__syncthreads();
		s_colx[1][idx] = s_colx[0][idx];
		s_coly[1][idx] = s_coly[0][idx];
		diag_h = up_h;
		left_h = cur_h;
		__syncthreads();
		_k--;
	}
	_k >>= 1; // we divide per 2 because we are loop-unrolling.
	for (; _k; _k--) {
		int4 cur_h;
		int up_h;
		int up_f;

		unsigned char s1;

		/* Loop-unrolling #1 */
		kernel_load<true>(idx, 1, j, busH, &up_h, &up_f, &s1);
		kernel_sw4<RECURRENCE_TYPE, FLUSH_LAST_ROW>(i, j, s1, ss, &left_e, &up_f, left_h, diag_h, up_h, &cur_h, flush_id);
		kernel_check_max4<CHECK_LOCATION, FLUSH_LAST_ROW>(i, j, max, max_pos, &cur_h, false, flush_id);
		kernel_flush<FLUSH_LAST_ROW>(i, j, idx, 0, busH, extraH, &cur_h, up_f, THREADS_COUNT-1, flush_id);
		j++;
		__syncthreads();

		/* Loop-unrolling #2 */
		kernel_load<true>(idx, 0, j, busH, &diag_h, &up_f, &s1);
		kernel_sw4<RECURRENCE_TYPE, FLUSH_LAST_ROW>(i, j, s1, ss, &left_e, &up_f, cur_h, up_h, diag_h, &left_h, flush_id);
		kernel_check_max4<CHECK_LOCATION, FLUSH_LAST_ROW>(i, j, max, max_pos, &left_h, false, flush_id);
		kernel_flush<FLUSH_LAST_ROW>(i, j, idx, 1, busH, extraH, &left_h, up_f, THREADS_COUNT-1, flush_id);
		j++;
		__syncthreads();

	}

	/* We store the result in the vertical bus to be read by the next block */

    busV_h[tidx]=left_h;
    busV_e[tidx]=left_e;
    busV_o[tidx].x=s_colx[1][idx];
    busV_o[tidx].y=s_coly[1][idx];
    busV_o[tidx].z=diag_h;

}

/**
 * This kernel processes all the internal diagonal during the long phase. The
 * short phase processes the first THREAD_COUNT-1 diagonals and the long phase
 * processes the remaining diagonals. Each internal diagonal are processed
 * by many threads in parallel, but with a synchronous barrier after each
 * internal diagonal.
 *
 * @tparam RECURRENCE_TYPE, CHECK_LOCATION
 * 				See these templates in the "Detailed Description" section in the beginning of this file.
 *
 * @param[in] i0,i1		the first and last row of the DP matrix
 * @param[in] step		the id of the external diagonal (0-based)
 * @param[in] cutBlock	(cutBlock.x, cutBlock.y) is the pruning window.
 * @param[in,out] blockResult 	stores the best score and its position for each block.
 * @param[in,out] busH		Horizontal bus used to transfer data between blocks (top-down).
 * @param[out] extraH		Extra Horizontal bus. See kernel_flush function for more information.
 * @param[in,out] busV_h,busV_e,busV_o		Vertical bus used to transfer data between blocks (left-right).
 */
//__launch_bounds__(THREADS_COUNT,MIN_BLOCKS_PER_SM)
// TODO testar com template FLUSH_LAST_ROW, pois ficou mais lento!
template <int RECURRENCE_TYPE, bool CHECK_MAX_SCORE>
__global__ void kernel_long_phase(
		const int i0, const int i1,
		const int step,
		const int2 cutBlock, int4 *blockResult,
		int2* busH, int2* extraH,
		int4* busV_h, int4* busV_e, int3* busV_o)
{
	const int bx = blockIdx.x;
	if (bx < cutBlock.x || bx > cutBlock.y) {
		// Block Pruning
		if (step - bx >= 0) {
			int tidx = ((step-bx) % gridDim.x)*THREADS_COUNT + threadIdx.x;
			busV_h[tidx]=make_int4(-INF,-INF,-INF,-INF);
			busV_e[tidx]=make_int4(-INF,-INF,-INF,-INF);
			busV_o[tidx]=make_int3(-INF,-INF,-INF);
			blockResult[bx].w = -INF;
		}
		return;
	}
	const int by = step-bx;
    if (by < 0) return;

    const int idx = threadIdx.x;

    const int x0 = d_split[bx]+(THREADS_COUNT-1);
    const int xLen = d_split[bx+1] - x0;

    const int tidx = (by % gridDim.x)*THREADS_COUNT+idx;
    const int i=((by*THREADS_COUNT)+idx)*ALPHA + i0;

    //const int j1 = d_split[gridDim.x];

    const int j=x0-idx;

    int2 max_pos;
    max_pos.x = -1;//blockResult[blockIdx.x].x;
    max_pos.y = -1;//blockResult[blockIdx.x].y;
    int max = -INF;//blockResult[bx].w;

        if (i >= i1) return;
       
	if (i < i1) {
		const int block_i = (by*THREADS_COUNT)*ALPHA + i0;
		if (block_i+THREADS_COUNT*ALPHA < i1) {
			// If the block is fully inside the range [i0..i1], then we do not request to flush the last row.
			process_internal_diagonals_long_phase<RECURRENCE_TYPE, CHECK_MAX_SCORE, false>(xLen, idx, tidx, i, j, busH, extraH, busV_h, busV_e, busV_o, &max, &max_pos, -1);
		} else {
			// Otherwise, we must flush the last row
			const int flush_id = get_flush_id(i, i0, i1);
			process_internal_diagonals_long_phase<RECURRENCE_TYPE, CHECK_MAX_SCORE, true >(xLen, idx, tidx, i, j, busH, extraH, busV_h, busV_e, busV_o, &max, &max_pos, flush_id);
		}
    }

	if (CHECK_MAX_SCORE) {
		if (findMax(threadIdx.x, max)) {
			blockResult[blockIdx.x].w = max;
			blockResult[blockIdx.x].x = max_pos.x;
			blockResult[blockIdx.x].y = max_pos.y+i;
			if (blockResult[blockIdx.x].z < max) {//max_pos.x != -1) {
				blockResult[blockIdx.x].z = max;
			}
		}
	} else {
    	blockResult[blockIdx.x].w = -INF;
	}
}

/**
 * This function processes the internal diagonals of the single phase (very small sequences).
 *
 * @tparam COLUMN_SOURCE, COLUMN_DESTINATION, RECURRENCE_TYPE, CHECK_LOCATION, FLUSH_LAST_ROW
 * 				See these templates in the "Detailed Description" section in the beginning of this file.
 *
 * @param[in] xLen	Number of internal diagonals.
 * @param[in] idx	Index of the current thread in the single block.
 * @param[in] i		The index of the row (top cell).
 * @param[in] j		The index of the column.
 * @param[in] i0,i1	the first and last row of the DP matrix
 * @param[in] j0,j1	the first and last column of the DP matrix
 * @param[in,out] busH		Horizontal bus used to transfer data between blocks (top-down).
 * @param[out] extraH		Extra Horizontal bus. See kernel_flush function for more information.
 * @param[in,out] busV_h,busV_e,busV_o		Vertical bus used to transfer data between blocks (left-right).
 * @param[in,out] max,max_pos	Best score and its position.
 * @param[in] flush_id	The index of the last DP row in the thread. See function get_flush_id.
 * @param[in] loadColumn_h,loadColumn_e		The values of the first column (when COLUMN_SOURCE=FROM_VECTOR).
 * @param[out] flushColumn_h,flushColumn_e		Stores the last column (when COLUMN_DESTINATION=TO_VECTOR).
 */
template<int COLUMN_SOURCE, int COLUMN_DESTINATION, int RECURRENCE_TYPE, int CHECK_LOCATION, bool FLUSH_LAST_ROW>
__device__ void process_internal_diagonals_single_phase(
		const int xLen, const int idx,
		int i, int j, const int i0, const int j0, const int i1, const int j1,
		int2* busH, int2* extraH,
		int4* busV_h, int4* busV_e, int3* busV_o,
		int* max, int2* max_pos,
		int flush_id,
		const int4* loadColumn_h, const int4* loadColumn_e,
		int4* flushColumn_h, int4* flushColumn_e)
{
    s_colx[0][idx] = s_colx[1][idx] = busV_o[idx].x;
    s_coly[0][idx] = s_coly[1][idx] = busV_o[idx].y;

	int4 left_h = busV_h[idx];
	int4 left_e = busV_e[idx];
    int  diag_h = busV_o[idx].z;

    uchar4 ss;
	fetchSeq0(i, &ss);

    __syncthreads();

    int _k = xLen;
    int index = 1;
    for (; _k; _k--) {
    	int4 curr_h;
        int  up_h;
        int  up_f;
		kernel_check_bound<COLUMN_SOURCE, COLUMN_DESTINATION>(i0, j0, i1, j1, &i, &j, &ss, &left_e, &diag_h, &left_h, &flush_id, loadColumn_h, loadColumn_e, flushColumn_h, flushColumn_e, idx, blockDim.x*ALPHA);
        if (flush_id) {
            unsigned char s1;
            kernel_load<false>(idx, index, j, busH, &up_h, &up_f, &s1);
			kernel_sw4<RECURRENCE_TYPE, FLUSH_LAST_ROW>(i, j, s1, ss, &left_e, &up_f, left_h, diag_h  , up_h  , &curr_h, flush_id);
			kernel_check_max4<CHECK_LOCATION, FLUSH_LAST_ROW>(i, j, max, max_pos, &curr_h, true, flush_id);
            kernel_flush<FLUSH_LAST_ROW>(i, j, idx, 1-index, busH, extraH, &curr_h, up_f, blockDim.x-1, flush_id);
        }
        index = 1-index;
        j++;
        diag_h = up_h;
        left_h = curr_h;
        __syncthreads();
    }
    kernel_check_bound<COLUMN_SOURCE, COLUMN_DESTINATION>(i0, j0, i1, j1, &i, &j, &ss, &left_e, &diag_h, &left_h, &flush_id, loadColumn_h, loadColumn_e, flushColumn_h, flushColumn_e, idx, blockDim.x*ALPHA);

    busV_h[idx]=left_h;
    busV_e[idx]=left_e;
    busV_o[idx].x=s_colx[index][idx];
    busV_o[idx].y=s_coly[index][idx];
    busV_o[idx].z=diag_h;

}

/**
 * This kernel processes all the internal diagonal with a single phase.
 * The single phase is used for very small sequences and only one block must
 * be executed.
 *
 * @tparam COLUMN_SOURCE, COLUMN_DESTINATION, RECURRENCE_TYPE, CHECK_LOCATION
 * 				See these templates in the "Detailed Description" section in the beginning of this file.
 *
 * @param[in] i0,i1		the first and last row of the DP matrix
 * @param[in] step		the id of the external diagonal (0-based)
 * @param[in] cutBlock	(cutBlock.x, cutBlock.y) is the pruning window.
 * @param[in,out] blockResult 	stores the best score and its position for each block.
 * @param[in,out] busH		Horizontal bus used to transfer data between external diagonals.
 * @param[out] extraH		Extra Horizontal bus. See kernel_flush function for more information.
 * @param[in,out] busV_h,busV_e,busV_o		Vertical bus used to transfer data between external diagonals.
 * @param[in] loadColumn_h,loadColumn_e		The values of the first column (when COLUMN_SOURCE=FROM_VECTOR).
 * @param[out] flushColumn_h,flushColumn_e		Stores the last column (when COLUMN_DESTINATION=TO_VECTOR).
 */
template <int COLUMN_SOURCE, int COLUMN_DESTINATION, int RECURRENCE_TYPE, int CHECK_LOCATION>
//__launch_bounds__(THREADS_COUNT,1)
__global__ void kernel_single_phase(
		const int i0, const int i1,
		const int step, const int2 cutBlock, int4 *blockResult,
		int2* busH, int2* extraH,
		int4* busV_h, int4* busV_e, int3* busV_o,
		const int4* loadColumn_h, const int4* loadColumn_e,
		int4* flushColumn_h, int4* flushColumn_e)
{

    int idx = threadIdx.x;
    int by = step;

	const int j0 = d_split[0];
	const int j1 = d_split[gridDim.x];

    const int xLen = j1-j0;
    int j=j0-idx-(xLen-blockDim.x+1); // This ensures that the block fills the busH entirely in the same row
    int i=(by*blockDim.x)+idx;
    i *= ALPHA;
    i += i0;

    if (j<=j0) { // TODO era while em vez de if
        j+=(j1-j0);
        i-=blockDim.x*ALPHA;
    }

    int2 max_pos;
    max_pos.x = -1;//blockResult[0].x;
    max_pos.y = -1;//blockResult[0].y;
    int max = -INF;//blockResult[0].w;

    int flush_id = get_flush_id(i, i0, i1);
	// TODO otimizar considerando warps
	if (i >= i1) return;

        if (i < i1) {
		const int block_i = (by*blockDim.x)*ALPHA +i0;
		//const int flush_id = get_flush_id(i, i0, i1);
		if (block_i+blockDim.x*ALPHA < i1) {
			// If the block is fully inside the range [i0..i1], then we do not request to flush the last row.
			process_internal_diagonals_single_phase<COLUMN_SOURCE, COLUMN_DESTINATION, RECURRENCE_TYPE, CHECK_LOCATION, false>(xLen, idx, i, j, i0, j0, i1, j1, busH, extraH, busV_h, busV_e, busV_o, &max, &max_pos, flush_id, loadColumn_h, loadColumn_e, flushColumn_h, flushColumn_e);
		} else {
			process_internal_diagonals_single_phase<COLUMN_SOURCE, COLUMN_DESTINATION, RECURRENCE_TYPE, CHECK_LOCATION, true>(xLen, idx, i, j, i0, j0, i1, j1, busH, extraH, busV_h, busV_e, busV_o, &max, &max_pos, flush_id, loadColumn_h, loadColumn_e, flushColumn_h, flushColumn_e);
		}
	}

    if (findMaxSmall(idx, max)) {
		blockResult[0].w = max;
		blockResult[0].x = max_pos.x;
		blockResult[0].y = max_pos.y;
        if (blockResult[0].z < max) {//max_pos.x != -1) {
            blockResult[0].z = max;
        }
    }
}

/**
 * Initialize the horizontal bus considering a zeroed first row (local alignment)
 * or an -infinity first row (prune block).
 *
 * @tparam H,F	the value of the H and F components to set in each row.
 *
 * @param[out] busH		horizontal bus.
 * @param[in] j0		first column to be processed.
 * @param[in] len		length of the range to be initialized (j0, j0+len).
 */
template<int H, int F>
static __global__ void kernel_initialize_busH_ungapped(int2* busH, const int j0, const int len) {
    int tidx = blockIdx.x*blockDim.x + threadIdx.x;
    while (tidx < len) {
		busH[j0+tidx].x = H;
        busH[j0+tidx].y = F;

		tidx += blockDim.x*gridDim.x;
    }
}

/**
 * Bind the textures for the DNA sequences.
 * @param seq0 sequence 0
 * @param seq0_len sequence 0 length
 * @param seq1 sequence 1
 * @param seq1_len sequence 1 length
 */
void bind_textures(const unsigned char* seq0, const int seq0_len, const unsigned char* seq1, const int seq1_len) {
	cutilSafeCall(cudaBindTexture(0, t_seq0, seq0, seq0_len));
	cutilSafeCall(cudaBindTexture(0, t_seq1, seq1, seq1_len));
}

/**
 * Unbind the textures for the DNA sequences.
 */
void unbind_textures() {
	cutilSafeCall(cudaUnbindTexture(t_seq1));
	cutilSafeCall(cudaUnbindTexture(t_seq0));
}

/**
 * Copies the split positions (used to identify the range of columns for each
 * block) to the GPU constant memory. The element split[0] must be the first
 * column of the partition and split[blocks] must be the last column.
 *
 * @param split vector with the block split positions.
 * @param blocks number of blocks.
 */
void copy_split(const int* split, const int blocks) {
	cutilSafeCall(cudaMemcpyToSymbol(d_split, split, (blocks+1)*sizeof(int)));
}

/**
 * Initialize the horizontal bus with H=-INF and F=-INF.
 *
 * @param p0,p1 range to be initialized.
 * @param d_busH bus to be initialized.
 */
void initializeBusHInfinity(const int p0, const int p1, int2* d_busH) {
	dim3 threads(512, 1, 1);
	dim3 blocks(MAX_BLOCKS_COUNT, 1, 1);
	kernel_initialize_busH_ungapped<-INF,-INF><<<threads, blocks>>>(d_busH, p0, p1-p0);
	cutilCheckMsg("Kernel execution failed");
}
//void CUDAligner::initializeBusHUngapped(const int p0, const int p1) {
//	dim3 threads(512, 1, 1);
//	dim3 blocks(BLOCKS_COUNT, 1, 1);
//	kernel_initialize_busH_ungapped<0,-INF><<<threads, blocks>>>(cuda.d_busH, p0, p1-p0);
//	cutilCheckMsg("Kernel execution failed");
//}

/****************************************************************************
 *
 * Templates function for creating precompiled different kernels.
 * Since we have four templates (COLUMN_SOURCE, COLUMN_DESTINATION,
 * RECURRENCE_TYPE, CHECK_LOCATION) with many options, we have
 * a total of 2x2x2x2 = 16 precompiled kernels with different
 * constant parameters. This improve significantly the
 * performance of the different kernel calls, but increases
 * compilation time and executable size.
 *
 ****************************************************************************/

/**
 * Invoke a kernel to process one external diagonal. Depending on the number
 * of blocks, it decides which kernel will be called: single phase or
 * short/long phase. The templates creates many precompiled kernels with
 * different constant parameters, instead of variable parameters. This increases
 * the performance with a tradeoff of creating 16 precompiled kernels (bigger
 * executable size).
 *
 * @tparam COLUMN_SOURCE, COLUMN_DESTINATION, RECURRENCE_TYPE, CHECK_LOCATION
 * 				See these templates in the "Detailed Description" section in the beginning of this file.
 * @param blocks Number of blocks. If blocks is equal to 1, then the
 * single phase kernel is called, otherwise it calls the short/long kernels.
 * @param threads Maximum number of threads per block.
 * @param i0 first column id.
 * @param i1 last column id.
 * @param step the current external diagonal id, starting from 0.
 * @param cutBlock pruning window.
 * @param cuda the object containing all the cuda allocated structures.
 */
template <int COLUMN_SOURCE, int COLUMN_DESTINATION, int RECURRENCE_TYPE, int CHECK_LOCATION>
void lauch_external_diagonals(const int blocks, const int threads,
		const int i0, const int i1,
		const int step, const int2 cutBlock, cuda_structures_t* cuda) {
	cutilSafeCall(cudaBindTexture(0, t_busH, cuda->d_busH, cuda->busH_size));
	dim3 grid( blocks, 1, 1);
	if (blocks == 1) {
		dim3 block( threads, 1, 1);
		kernel_single_phase<COLUMN_SOURCE, COLUMN_DESTINATION, RECURRENCE_TYPE, CHECK_LOCATION><<< grid, block, 0>>>(i0, i1, step, cutBlock, cuda->d_blockResult, cuda->d_busH, cuda->d_extraH, cuda->d_busV_h, cuda->d_busV_e, cuda->d_busV_o, cuda->d_loadColumnH, cuda->d_loadColumnE, cuda->d_flushColumnH, cuda->d_flushColumnE);
	} else {
		static dim3 block( THREADS_COUNT, 1, 1);
		kernel_long_phase<RECURRENCE_TYPE, CHECK_LOCATION><<< grid, threads, 0>>>(i0, i1, step-1, cutBlock, cuda->d_blockResult, cuda->d_busH, cuda->d_extraH, cuda->d_busV_h, cuda->d_busV_e, cuda->d_busV_o);
		kernel_short_phase<COLUMN_SOURCE, COLUMN_DESTINATION, RECURRENCE_TYPE, CHECK_LOCATION><<< grid, threads, 0>>>(i0, i1, step, cutBlock, cuda->d_blockResult, cuda->d_busH, cuda->d_extraH, cuda->d_busV_h, cuda->d_busV_e, cuda->d_busV_o, cuda->d_loadColumnH, cuda->d_loadColumnE, cuda->d_flushColumnH, cuda->d_flushColumnE);
	}
	cudaStreamSynchronize(0);
	cutilCheckMsg("Kernel execution failed");
	cutilSafeCall(cudaUnbindTexture(t_busH));
}


/* Templated-inline Function */
template <int COLUMN_SOURCE, int COLUMN_DESTINATION, int RECURRENCE_TYPE>
void lauch_external_diagonals(int CHECK_LOCATION, const int blocks, const int threads,
		const int i0, const int i1,
		const int step, const int2 cutBlock, cuda_structures_t* cuda) {
	if (CHECK_LOCATION) {
		lauch_external_diagonals<COLUMN_SOURCE, COLUMN_DESTINATION, RECURRENCE_TYPE, CHECK_BEST_SCORE>(blocks, threads, i0, i1, step, cutBlock, cuda);
	} else {
		lauch_external_diagonals<COLUMN_SOURCE, COLUMN_DESTINATION, RECURRENCE_TYPE, IGNORE_BEST_SCORE>(blocks, threads, i0, i1, step, cutBlock, cuda);
	}
}

/* Templated-inline Function */
template <int COLUMN_SOURCE, int COLUMN_DESTINATION>
void lauch_external_diagonals(int RECURRENCE_TYPE, int CHECK_LOCATION, const int blocks, const int threads,
		const int i0, const int i1,
		const int step, const int2 cutBlock, cuda_structures_t* cuda) {
	if (RECURRENCE_TYPE == SMITH_WATERMAN) {
		lauch_external_diagonals<COLUMN_SOURCE, COLUMN_DESTINATION, SMITH_WATERMAN>(CHECK_LOCATION, blocks, threads, i0, i1, step, cutBlock, cuda);
	} else if (RECURRENCE_TYPE == NEEDLEMAN_WUNSCH) {
		lauch_external_diagonals<COLUMN_SOURCE, COLUMN_DESTINATION, NEEDLEMAN_WUNSCH>(CHECK_LOCATION, blocks, threads, i0, i1, step, cutBlock, cuda);
	} else {
		// DIE
	}
}


/* Templated-inline Function */
template <int COLUMN_SOURCE>
void lauch_external_diagonals(int COLUMN_DESTINATION, int RECURRENCE_TYPE, int CHECK_LOCATION, const int blocks, const int threads,
		const int i0, const int i1,
		const int step, const int2 cutBlock, cuda_structures_t* cuda) {
	if (COLUMN_DESTINATION) {
		lauch_external_diagonals<COLUMN_SOURCE, STORE_LAST_COLUMN>(RECURRENCE_TYPE, CHECK_LOCATION, blocks, threads, i0, i1, step, cutBlock, cuda);
	} else {
		lauch_external_diagonals<COLUMN_SOURCE, DISCARD_LAST_COLUMN>(RECURRENCE_TYPE, CHECK_LOCATION, blocks, threads, i0, i1, step, cutBlock, cuda);
	}
}

/* Templated-inline Function */
void lauch_external_diagonals(int COLUMN_SOURCE, int COLUMN_DESTINATION, int RECURRENCE_TYPE, int CHECK_LOCATION, const int blocks, const int threads,
		const int i0, const int i1,
		const int step, const int2 cutBlock, cuda_structures_t* cuda) {
	switch (COLUMN_SOURCE) {
		case INIT_WITH_ZEROES:
			lauch_external_diagonals<FROM_ZEROES>(COLUMN_DESTINATION, RECURRENCE_TYPE, CHECK_LOCATION, blocks, threads, i0, i1, step, cutBlock, cuda);
			break;
		default:
			lauch_external_diagonals<FROM_VECTOR>(COLUMN_DESTINATION, RECURRENCE_TYPE, CHECK_LOCATION, blocks, threads, i0, i1, step, cutBlock, cuda);
			break;
	}
}




