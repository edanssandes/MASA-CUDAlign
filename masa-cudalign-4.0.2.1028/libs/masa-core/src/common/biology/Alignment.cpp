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

#include <vector>

#include "Alignment.hpp"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <algorithm>

#include "AlignmentBinaryFile.hpp"

//#define USE_CAIRO

Alignment::Alignment(AlignmentParams* params) : params(params) {
	alignmentFinalized = false;
}

Alignment::Alignment(const Alignment& orig) {
	alignmentFinalized = false;	
}

Alignment::~Alignment() {
	gap_sequences.clear();
}

void Alignment::addGapInSeq0(int i) {
    addGap(0, i);
}

void Alignment::addGapInSeq1(int j) {
    addGap(1, j);
}

vector<gap_sequence_t>* Alignment::getGapSequences() {
	if (gap_sequences.size() > 0) {
		return &gap_sequences;
	}
	
	int i0=this->start[0];
	int j0=this->start[1];
	int i1=this->end[0];
	int j1=this->end[1];

	int dir_i = (i1 > i0) ? +1 : -1;
	int dir_j = (j1 > j0) ? +1 : -1;
	
	int c0 = (dir_i > 0) ? 0 : gaps[0].size()-1;
	int c1 = (dir_j > 0) ? 0 : gaps[1].size()-1;
	
	int next_gap_i = (c0<0 || c0>=gaps[0].size()) ? -1 : gaps[0][c0].pos;
	int next_gap_j = (c1<0 || c1>=gaps[1].size()) ? -1 : gaps[1][c0].pos;//FIXME c0??? não deveria ser c1??
	
	gap_sequences.resize(gaps[0].size() + gaps[1].size());
	int k=0;
	int i=i0;
	int j=j0;
	while (next_gap_i != -1 || next_gap_j != -1) {
		// Calcula a distancia do ponto atual até o proxy gap do tipo 0 e do tipo 1
		int diff_i = next_gap_i-i;
		int diff_j = next_gap_j-j;

		/*printf("(%d %d) (%d %d) = (%d %d) (%d:%d %d:%d)\n", 
			   i, j, next_gap_i, next_gap_j, diff_i, diff_j, c0, gaps[0][c0].len, c1, gaps[1][c1].len);*/
		gap_sequence_t gap_sequence;
		
		// Processa o gap mais proximo
		if (abs(diff_j) < abs(diff_i)) {
			i += diff_j;
			j = next_gap_j;
			gap_sequences[k].i0 = i;
			gap_sequences[k].j0 = j;
			i += gaps[1][c1+=dir_j].len;
		} else {
			i = next_gap_i;
			j += diff_i;
			gap_sequences[k].i0 = i;
			gap_sequences[k].j0 = j;
			j += gaps[0][c0+=dir_i].len;
		}
		gap_sequences[k].i1 = i;
		gap_sequences[k].j1 = j;
		k++;

		next_gap_i = (c0<0 || c0>=gaps[0].size()) ? -1 : gaps[0][c0].pos;
		next_gap_j = (c1<0 || c1>=gaps[1].size()) ? -1 : gaps[1][c0].pos;
	}
	printf("%d/%d: (%d %d) (%d %d)\n", k, gaps[0].size() + gaps[1].size(), i, j, end[0], end[1]);
	printf("count: %d\n", gap_sequences.size());	
	
	return &gap_sequences;
}

bool myfunction (gap_t a,gap_t b) { return (a.pos<b.pos); }

void Alignment::finalize(/*int i0, int j0, int i1, int j1*/) {
    /*this->start[0] = i0;
    this->start[1] = j0;
    this->end[0] = i1;
    this->end[1] = j1;*/
	this->alignmentFinalized = true;

	for (int i=0; i<params->getSequencesCount(); i++) {
		sort(gaps[i].begin(), gaps[i].end(), myfunction);
	}
}


bool Alignment::isFinalized() {
	return alignmentFinalized;
}

/*void Alignment::printBinary(string filename) {
	AlignmentBinaryFile::write(filename, this);
}

void Alignment::loadBinary(string filename) {
	AlignmentBinaryFile::read(filename, this);
}*/

AlignmentParams* Alignment::getAlignmentParams() const {
	return params;
}

int Alignment::getRawScore() const {
	return rawScore;
}

void Alignment::setRawScore(int rawScore) {
	this->rawScore = rawScore;
}

void Alignment::setStart(int seq, int pos) {
	start[seq] = pos;
}

void Alignment::setEnd(int seq, int pos) {
	end[seq] = pos;
}

int Alignment::getStart(int seq) {
	return start[seq];
}

int Alignment::getEnd(int seq) {
	return end[seq];
}

string Alignment::getPruningFile() const {
	return pruningFile;
}

void Alignment::setPruningFile(string pruningFile) {
	this->pruningFile = pruningFile;
}

int Alignment::getGapExtensions() const {
	return gapExtensions;
}

void Alignment::setGapExtensions(int gapExtensions) {
	this->gapExtensions = gapExtensions;
}

int Alignment::getGapOpen() const {
	return gapOpen;
}

void Alignment::setGapOpen(int gapOpen) {
	this->gapOpen = gapOpen;
}

int Alignment::getMatches() const {
	return matches;
}

void Alignment::setMatches(int matches) {
	this->matches = matches;
}

int Alignment::getMismatches() const {
	return mismatches;
}

void Alignment::setMismatches(int mismatches) {
	this->mismatches = mismatches;
}

void Alignment::addGap(int seq, int i) {
    if (gaps[seq].size() != 0 && gaps[seq].back().pos == i) {
        gaps[seq].back().len++;
        //printf("-seq: %d  i: %d  (%d)   [%d]\n", seq, i, gaps[seq].back().len, gaps[seq].size());
    } else {
        gap_t new_gap(i, 1);
        new_gap.pos = i;
        new_gap.len = 1;
        gaps[seq].push_back(new_gap);
        //printf("+seq: %d  i: %d  (%d)   [%d]\n", seq, i, gaps[seq].back().len, gaps[seq].size());
    }

    /*int* c = &gaps[seq][0];
    int* s = &gaps[seq][(*c)*2+1];
    int* e = &gaps[seq][(*c)*2+2];

    //printf("seq: %d  i: %d  [%d/%d]\n", seq, i, *s, *e);
    if (*c>0 && i == *s) {
        (*e)++;
    } else {
        (*c)++;
        gaps[seq][(*c)*2+1] = i;
        gaps[seq][(*c)*2+2] = 1;
    } */
}
