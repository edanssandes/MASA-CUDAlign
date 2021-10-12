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

#ifndef CONSTANTS_HPP_
#define CONSTANTS_HPP_


#define FLAG_CLEAR_N	0x0004
#define FLAG_COMPLEMENT	0x0002
#define FLAG_REVERSE	0x0001

#define SEQUENCE_TYPE_DNA				(1)
#define SEQUENCE_TYPE_RNA				(2)
#define SEQUENCE_TYPE_PROTEIN			(3)
#define SEQUENCE_TYPE_UNKNOWN			(255)


#define ALIGNMENT_METHOD_GLOBAL		(1)
#define ALIGNMENT_METHOD_LOCAL		(2)

#define PENALTY_LINEAR_GAP			(1)
#define PENALTY_AFFINE_GAP			(2)

#define SCORE_MATCH_MISMATCH		(1)
#define SCORE_SIMILARITY_MATRIX		(2)


#endif /* CONSTANTS_HPP_ */
