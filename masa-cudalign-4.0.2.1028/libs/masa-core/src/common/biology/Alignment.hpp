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

class Alignment;

#ifndef _ALIGNMENT_HPP
#define	_ALIGNMENT_HPP

#include <string>
using namespace std;

#include "AlignmentParams.hpp"

#define TYPE_MATCH	(0)
#define TYPE_GAP_1	(1) //HORIZONTAL (gap on sequence 0) // TODO alterar constantes
#define TYPE_GAP_2	(2) //VERTICAL (gap on sequence 1) // TODO alterar constantes

struct gap_sequence_t {
	int i0;
	int j0;
	int i1;
	int j1;
	int type() {
		if (i0==i1) return 1;
		if (j0==j1) return 2;
		return 0; //TODO ERROR
	}
	
	int len() {
		return (i1-i0)+(j1-j0);
	}
};	

struct gap_t {
	int pos;
	int len;

	gap_t() {

	}

	gap_t(int pos, int len) {
		this->pos = pos;
		this->len = len;
	}
};

class Alignment {
public:
	
    Alignment(AlignmentParams* params);
    Alignment(const Alignment& orig);
    virtual ~Alignment();

    void addGapInSeq0(int i);
    void addGapInSeq1(int j);
    void finalize /*int i0, int j0, int i1, int j1*/
	();
	bool isFinalized();
	//void printBinary(string filename);
	//void loadBinary(string filename);
	vector<gap_sequence_t>* getGapSequences();
	AlignmentParams* getAlignmentParams() const;
	int getRawScore() const;
	void setRawScore(int rawScore);

	vector<gap_t>* getGaps(int seq) {
		return &gaps[seq];
	}

	void setStart(int seq, int pos);
	void setEnd(int seq, int pos);
	int getStart(int seq);
	int getEnd(int seq);
	string getPruningFile() const;
	void setPruningFile(string pruningFile);
	int getGapExtensions() const;
	void setGapExtensions(int gapExtensions);
	int getGapOpen() const;
	void setGapOpen(int gapOpen);
	int getMatches() const;
	void setMatches(int matches);
	int getMismatches() const;
	void setMismatches(int mismatches);

private:
	int start[2];
	int end[2];

	int rawScore;
	int matches;
	int mismatches;
	int gapOpen;
	int gapExtensions;
	
	bool alignmentFinalized;
	AlignmentParams* params;

    vector<gap_t> gaps[2];
	vector<gap_sequence_t> gap_sequences;
	string pruningFile;

    void addGap(int seq, int i);

};

#endif	/* _ALIGNMENT_HPP */

