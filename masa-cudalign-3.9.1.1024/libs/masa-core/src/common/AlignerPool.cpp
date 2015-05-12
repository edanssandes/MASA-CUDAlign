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

#include "AlignerPool.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>

AlignerPool::AlignerPool(string sharedPath) {
	this->sharedPath = sharedPath;
	this->crosspointIdSentCounter = 0;
	this->crosspointIdRecvCounter = 0;
	this->bestNodeScore.score = -INF;
	this->bestNodeScore.i = -1;
	this->bestNodeScore.j = -1;
}

AlignerPool::~AlignerPool() {

}

void AlignerPool::waitId(int id) {
	string filename = getMsgFile("stage1", id);
	waitSignal(filename);
}

void AlignerPool::dispatchScore(score_t score) {
	string filename = getMsgFile("stage1", right);

	FILE* file = fopen(filename.c_str(), "wt");
	fprintf(file, "%d %d %d\n", score.i, score.j, score.score);
	fclose(file);

	sendSignal(filename);

}

score_t AlignerPool::receiveScore() {
	score_t score;

	string filename = getMsgFile("stage1", left);
	waitSignal(filename);

	FILE* file = fopen(filename.c_str(), "rt");
	fscanf(file, "%d %d %d\n", &score.i, &score.j, &score.score);
	fclose(file);
	return score;

}

void AlignerPool::dispatchCrosspoint(crosspoint_t crosspoint, int final) {
	string filename = getMsgFile("stage2", left, crosspointIdSentCounter++);

	FILE* file = fopen(filename.c_str(), "wt");
	fprintf(file, "%d %d %d %d %d\n", crosspoint.i, crosspoint.j, crosspoint.score, crosspoint.type, final);
	fclose(file);

	sendSignal(filename);
}

crosspoint_t AlignerPool::receiveCrosspoint(int* final) {
	crosspoint_t c;
	bool _final;

	string filename = getMsgFile("stage2", right, crosspointIdRecvCounter++);
	waitSignal(filename);

	while (peekSignal(getMsgFile("stage2", right, crosspointIdRecvCounter))) {
		filename = getMsgFile("stage2", right, crosspointIdRecvCounter++);
	}

	FILE* file = fopen(filename.c_str(), "rt");
	fscanf(file, "%d %d %d %d %d\n", &c.i, &c.j, &c.score, &c.type, &_final);
	fclose(file);

	if (final != NULL) {
		*final = _final;
	}

	return c;
}


void AlignerPool::dispatchCrosspointFile(CrosspointsFile* file) {
	string filename = getMsgFile("stage4", left);

	file->writeToFile(filename.c_str());
	sendSignal(filename);

}

CrosspointsFile* AlignerPool::receiveCrosspointFile() {
	string filename = getMsgFile("stage4", right);
	waitSignal(filename);

	CrosspointsFile* crosspoints = new CrosspointsFile(filename);
	crosspoints->loadCrosspoints();

	return crosspoints;
}

void AlignerPool::registerNode(int id, int left, int right, string flushURL) {
	string filename = getMsgFile("register", id);
	this->left = left;
	this->right = right;

	printf("%d: %d,%d\n", id, left, right);
	FILE* file = fopen(filename.c_str(), "wt");
	fprintf(file, "%s\n", flushURL.c_str());
	fclose(file);

	sendSignal(filename);
}

string AlignerPool::getLoadURL(int id) {
	string filename = getMsgFile("register", id);
	waitSignal(filename);

	FILE* file = fopen(filename.c_str(), "rt");
	char url[200];
	fscanf(file, "%s\n", url);
	fclose(file);

	return string(url);
}



void AlignerPool::initialize() {

}

string AlignerPool::getMsgFile(string prefix, int id, int count) {
	char str[100];
	if (count == -1) {
		sprintf(str, ".%08X", id);
	} else {
		sprintf(str, ".%08X.%02d", id, count);
	}
	return this->sharedPath + "/" + prefix + string(str);
}

string AlignerPool::getSignalFile(string msgFile) {
	return msgFile + ".signal";
}

void AlignerPool::sendSignal(string msgFile) {
	string signalFile = getSignalFile(msgFile);
	FILE* file = fopen(signalFile.c_str(), "wt");
	fclose(file);
	sync();
	printf("[%d] Signal Sent: %s\n", getpid(), signalFile.c_str());
}

bool AlignerPool::isFirstNode() {
	return left == -1;
}

bool AlignerPool::isLastNode() {
	return right == -1;
}

const score_t& AlignerPool::getBestNodeScore() const {
	return bestNodeScore;
}

void AlignerPool::setBestNodeScore(const score_t& bestNodeScore) {
	this->bestNodeScore = bestNodeScore;
}

bool AlignerPool::peekSignal(string msgFile) {
	string signalFile = getSignalFile(msgFile);
	FILE* file = fopen(signalFile.c_str(), "rt");
	bool signalOk = false;
	if (file != NULL) {
		signalOk = true;
		fclose(file);
	}
	return signalOk;
}

void AlignerPool::waitSignal(string msgFile) {
	int count = 0;
	bool signalOk = false;
	while (!signalOk) {
		if (count % 1000 == 0) {
			printf("[%d] Waiting Signal for msg: %s\n", getpid(), msgFile.c_str());
		}
		signalOk = peekSignal(msgFile);
		if (!signalOk) {
			usleep(10000);
			count++;
		}
	}
	printf("[%d] Signal Received for msg: %s\n", getpid(), msgFile.c_str());
}

