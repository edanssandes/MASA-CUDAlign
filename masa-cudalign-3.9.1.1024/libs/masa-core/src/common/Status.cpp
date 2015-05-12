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

#include "Status.hpp"

#include <stdio.h>
#include <stdlib.h>

Status::Status(string path, BestScoreList* bestScoreList) {
	status_file = path + "/status";
	status_tmp = path + "/status.tmp";
	this->bestScoreList = bestScoreList;
	this->lastSpecialRow = 0;
	this->currentStage = 0;

	load();
}

Status::~Status() {
	save();
}

int Status::isEmpty() {
	return empty;
}

void Status::load() {
    file = fopen(status_file.c_str(), "rt");
	if (file == NULL) {
	    file = fopen(status_tmp.c_str(), "rt");
		if (file == NULL) {
			printf("Empty!!\n");
			empty = true;
			return;
		}
		rename(status_tmp.c_str(), status_file.c_str());
	}
	empty = false;
	int i;
	int j;
	int score;
	fscanf(file, "%d", &currentStage);
	fscanf(file, "%d", &lastSpecialRow);
	while (fscanf(file, "%d%d%d", &i, &j, &score) == 3) {
		bestScoreList->add(i, j, score);
	}

	printf("NOT Empty: %d!!\n", score);
	fclose(file);
}

void Status::save() {
    file = fopen(status_tmp.c_str(), "wt");
	if (file == NULL) {
		fprintf(stderr, "Error opening status file: %s\n", status_tmp.c_str());
		exit(1);
	}
	fprintf(file, "%d\n", currentStage);
	fprintf(file, "%d\n", lastSpecialRow);
	// FIXME This is not thread safe!!!!
//	for (set<score_t>::iterator it=bestScoreList->begin() ; it != bestScoreList->end(); it++ ) {
//		fprintf(file, "%d %d %d\n", it->i, it->j, it->score);
//	}
	score_t best = bestScoreList->getBestScore();
	fprintf(file, "%d %d %d\n", best.i, best.j, best.score);

	fclose(file);

	remove(status_file.c_str());
	rename(status_tmp.c_str(), status_file.c_str());
}

int Status::getLastSpecialRow() const {
	return lastSpecialRow;
}

void Status::setLastSpecialRow(int lastSpecialRow) {
	this->lastSpecialRow = lastSpecialRow;
}

int Status::getCurrentStage() const {
	return currentStage;
}

void Status::setCurrentStage(int currentStage) {
	this->currentStage = currentStage;
}
