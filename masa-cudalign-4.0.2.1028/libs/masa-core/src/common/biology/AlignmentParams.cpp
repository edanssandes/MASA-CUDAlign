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

#include "AlignmentParams.hpp"

#include <stdio.h>
#include "Constants.hpp"

AlignmentParams::AlignmentParams() {

}

AlignmentParams::~AlignmentParams() {

}

void AlignmentParams::swapSequences() {
	if (sequences.size() == 2) {
		Sequence* tmp = sequences[1];
		sequences[1] = sequences[0];
		sequences[0] = tmp;
	}
}

Sequence* AlignmentParams::getSequence(int id) {
	return sequences[id];
}

vector<Sequence*> AlignmentParams::getSequences() {
	return sequences;
}

int AlignmentParams::getAlignmentMethod() const {
	return alignmentMethod;
}

void AlignmentParams::setAlignmentMethod(int alignmentMethod) {
	this->alignmentMethod = alignmentMethod;
}


void AlignmentParams::setAffineGapPenalties(int gapOpen, int gapExtension) {
    setPenaltySystem(PENALTY_AFFINE_GAP);
	this->gapOpen = gapOpen;
	this->gapExtension = gapExtension;
}

int AlignmentParams::getGapOpen() const {
	return gapOpen;
}

int AlignmentParams::getGapExtension() const {
	return gapExtension;
}

void AlignmentParams::setMatchMismatchScores(int match, int mismatch) {
    setScoreSystem(SCORE_MATCH_MISMATCH);
    this->match = match;
	this->mismatch = mismatch;
}

int AlignmentParams::getMatch() const {
	return match;
}

int AlignmentParams::getMismatch() const {
	return mismatch;
}

int AlignmentParams::getPenaltySystem() const {
	return penaltySystem;
}

void AlignmentParams::setPenaltySystem(int penaltySystem) {
	this->penaltySystem = penaltySystem;
}

int AlignmentParams::getScoreSystem() const {
	return scoreSystem;
}

void AlignmentParams::setScoreSystem(int scoreSystem) {
	this->scoreSystem = scoreSystem;
}

int AlignmentParams::getSequencesCount() const {
	return sequences.size();
}

void AlignmentParams::addSequence(Sequence* sequence) {
	sequences.push_back(sequence);
}

void AlignmentParams::printParams(FILE* file) {
	fprintf(file, "SW PARAM: %d/%d/%d/%d\n", match, mismatch, gapOpen, gapExtension);

	fprintf(file, "--Alignment sequences:\n");
	for (int i=0; i<sequences.size(); i++) {
		fprintf(file, ">%s (%d)\n", sequences[i]->getInfo()->getAccession().c_str(), sequences[i]->getLen());
	}
}
