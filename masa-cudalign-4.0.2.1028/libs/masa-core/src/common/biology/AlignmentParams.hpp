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

#ifndef ALIGNMENTPARAMS_H_
#define ALIGNMENTPARAMS_H_

#include "Sequence.hpp"

class AlignmentParams {
public:
	AlignmentParams();
	virtual ~AlignmentParams();

	Sequence* getSequence(int id);
	vector<Sequence*> getSequences();
	void addSequence(Sequence* sequence);
	void swapSequences();

	int getAlignmentMethod() const;
	void setAlignmentMethod(int alignmentMethod);

	int getGapOpen() const;
	int getGapExtension() const;
	void setAffineGapPenalties(int gapOpen, int gapExtension);

	int getMatch() const;
	int getMismatch() const;
	void setMatchMismatchScores(int match, int mismatch);

	int getPenaltySystem() const;
	void setPenaltySystem(int penaltySystem);

	int getScoreSystem() const;
	void setScoreSystem(int scoreSystem);

	int getSequencesCount() const;

	void printParams(FILE* file);
private:
	vector<Sequence*> sequences;

	int alignmentMethod;
	int penaltySystem;
	int scoreSystem;

	int match;
	int mismatch;

	int gapOpen;
	int gapExtension;

};

#endif /* ALIGNMENTPARAMS_H_ */
