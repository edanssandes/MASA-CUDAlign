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

#ifndef CROSSPOINT_HPP_
#define CROSSPOINT_HPP_

// TODO esse include foi colocado somente por causa do TYPE. Talvez valha a pena mover essa definição de lugar.
#include "biology/Alignment.hpp"

/** @brief Represents a crosspoint between the optimal alignment and a special row.
 *
 * A crosspoint is a coordinate of the optimal alignment that crosses some
 * special row or special column. A crosspoint is represented by a tuple
 * (i, j, score, type), where score is the score of the alignment in position
 * (i,j) and type is the type of the alignment in this position. Type can be
 * TYPE_MATCH in case of match or mismatch, TYPE_GAP_1 if there is a gap in
 * sequence S_0 and TYPE_GAP_2 if there is a gap in sequence S_1.
 */
typedef struct crosspoint_t {
	/** vertical coordinate */
    int i;

    /** horizontal coordinate */
    int j;

    /** type of the crosspoint. It can be TYPE_MATCH, TYPE_GAP_1 or TYPE_GAP_2. */
    int type;

    /** The score of this crosspoint considering since the start of the alignment. */
    int score;

    /**
     * Reverse the crosspoint considering the reversed sequences. The reverse
     * procedure changes the original (i,j) coordinates to new (i', j') coordinates
     * using the following equations:
     * \f{eqnarray*}{
     *   i' &=& |S_1|-j \\
     *   j' &=& |S_0|-i
     * \f}
     * The type of the crosspoint is changed between TYPE_GAP_1 and TYPE_GAP_2.
     * The score is not changed.
     *
     * @param seq0_len The length \f$|S_0|\f$ of the sequence 0 (related to the i coordinate).
     * @param seq1_len The length \f$|S_1|\f$ of the sequence 1 (related to the j coordinate).
     * @return the new crosspoint with the reversed sequences.
     */
    crosspoint_t reverse(int seq0_len, int seq1_len) {
    	crosspoint_t ret;
		ret.i = seq1_len-this->j;
		ret.j = seq0_len-this->i;
		ret.score = this->score;
		if (this->type == TYPE_GAP_1) {
			ret.type = TYPE_GAP_2;
		} else if (this->type == TYPE_GAP_2) {
			ret.type = TYPE_GAP_1;
		} else {
			ret.type = this->type;
		}
		return ret;
	}

    bool operator==(const crosspoint_t &other) const {
    	return (this->i==other.i) && (this->j==other.j) && (this->type==other.type) && (this->score==other.score);
    }

    bool operator!=(const crosspoint_t &other) const {
      return !(*this == other);
    }
} crosspoint_t;


#endif /* CROSSPOINT_HPP_ */
