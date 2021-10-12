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

#ifndef LIBMASATYPES_HPP_
#define LIBMASATYPES_HPP_


/**
 * Struct that represents a cell in the DP matrix. See the Smith-Waterman
 * recurrence function with the gotoh affine gap modification.
 *
 * Each cell has three components: H, E and F. But note that the E and F
 * components holds the same memory area, because the E and F components are a
 * union. So, the cell structure may represent only two components simultaneously,
 * the \f$(H,F)\f$ components or the \f$(H,E)\f$ components.
 */
typedef struct {
	int h;
	union {
		int f;
		int e;
	};
} __attribute__ ((aligned (8))) cell_t;

/**
 * Infinity number used in the cells of the DP matrix.
 */
#define INF (999999999)

/**
 * Struct that notifies the match result in the Mayers-Miller matching procedure.
 */
typedef struct {
	/** true if the matching procedure found the goal score */
	bool found;
	/** index of the matched score */
	int k;
	/** matched score */
	int score;
	/** type of the match: MATCH_ALIGNED or MATCH_GAPPED */
	int type;
} match_result_t;

/**
 * Indicates that the matching procedure found the goal score adding the H
 * component of both rows.
 */
#define MATCH_ALIGNED	(0)

/**
 * Indicates that the matching procedure found the goal score adding the E
 * component of both rows (minus the GAP_OPENING penalty).
 */
#define MATCH_GAPPED	(1)

/**
 * Indicates that the matching procedure failed with (sum_match > goalScore)
 */
#define MATCH_ERROR_1 (-1)

/**
 * Indicates that the matching procedure failed with (sum_gap > goalScore)
 */
#define MATCH_ERROR_2 (-2)


/**
 * Represents a score in the DP matrix.
 */
typedef struct {
	/** i-coordinate of the score */
	int i;
	/** j-coordinate of the score */
	int j;
	/** value of the score */
	int score;
} score_t;

/**
 * Defines the match/mismatch score and affine gap penalties.
 */
typedef struct {
	/** match score (positive). */
	int match;
	/** mismatch penalty (negative). */
	int mismatch;
	/** gap opening penalty (positive). */
	int gap_open;
	/** gap extension penalty (positive). */
	int gap_ext;
} score_params_t;


#endif /* LIBMASATYPES_HPP_ */
