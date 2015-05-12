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

#include "CrosspointsFile.hpp"

#include <stdlib.h>
#include <string.h>
#include <algorithm>
using namespace std;

CrosspointsFile::CrosspointsFile(string filename) {
    this->filename = filename;
    this->tmpFilename = filename + ".tmp";
    this->autoSave = false;
    this->file = NULL;
}

CrosspointsFile::~CrosspointsFile() {
	close();
}

void CrosspointsFile::loadCrosspoints() {
    FILE * file = fopen(filename.c_str(), "r");
    if (file == NULL) {
    	this->clear();
        printf("NO CROSSPOINTS: %s\n", filename.c_str());
    	return;
    }

    char line[500];
    int started = 0;
    while (fgets(line, sizeof(line), file) != NULL) {
        if (strcmp(line, "END\n")==0) break;
        if (started) {
            crosspoint_t temp;
            sscanf(line, "%d,%d,%d,%d", &temp.type, &temp.i, &temp.j, &temp.score);
            this->push_back(temp);

        }
        if(strcmp(line, "START\n")==0) {
            started = 1;
            this->clear();
        }
    }
    fclose(file);

	printf("LOAD CROSSPOINTS: count %d\n", this->size());
    /*if (this->front().i < this->back().i || this->front().j < this->back().j) {
        std::reverse(this->begin(),this->end());
    }*/
}

int CrosspointsFile::getLargestPartitionSize(int* max_i, int* max_j) {
    int max_size_i = 0;
	int max_size_j = 0;
	for (int i=1; i < this->size(); i++) {
        crosspoint_t curr = this->at(i);
        crosspoint_t prev = this->at(i-1);
        int delta_i = abs(prev.i-curr.i);
        int delta_j = abs(prev.j-curr.j);
        if (delta_i != 0 && delta_j != 0) {
        	// partitions with full gap are discarded (trivial case)
			if (max_size_i < delta_i) max_size_i = delta_i;
			if (max_size_j < delta_j) max_size_j = delta_j;
        }
    }
	if (max_i != NULL) *max_i = max_size_i;
	if (max_j != NULL) *max_j = max_size_j;
	if (max_size_i > max_size_j) {
		return max_size_i;
	} else {
		return max_size_j;
	}
}

void CrosspointsFile::setAutoSave() {
	autoSave = true;
	open();
}

void CrosspointsFile::writeToFile(string filename) {
	FILE* file = fopen(filename.c_str(), "w");
	fprintf(file, "START\n");
	for (int i=0; i < this->size(); i++) {
		crosspoint_t c = at(i);
		fprintf(file, "%d,%d,%d,%d\n", c.type, c.i, c.j, c.score);
	}
	fprintf(file, "END\n");
    fclose(file);
}

void CrosspointsFile::open() {
    file = fopen(tmpFilename.c_str(), "w");
	if (file == NULL) {
		fprintf(stderr, "Error opening crosspoints file: %s\n", tmpFilename.c_str());
		exit(1);
	}
    fprintf(stderr, "START\n");
    fprintf(file, "START\n");
}

void CrosspointsFile::close() {
    if (file != NULL) {
        fprintf(stderr, "END\n");
        fprintf(file, "END\n");

        fclose(file);
        file = NULL;

        rename(tmpFilename.c_str(), filename.c_str());
    }
}

void CrosspointsFile::write(int i, int j, int score, int type) {
	crosspoint_t crosspoint;
	crosspoint.i = i;
	crosspoint.j = j;
	crosspoint.score = score;
	crosspoint.type = type;
	write(crosspoint);
}

void CrosspointsFile::write(crosspoint_t crosspoint) {
	if (file == NULL) {
		fprintf(stderr, "Crosspoints File not opened.\n");
		exit(1);
	}
    fprintf(file, "%d,%d,%d,%d\n", crosspoint.type, crosspoint.i, crosspoint.j, crosspoint.score);
    fprintf(stderr, "%d,%d,%d,%d\n", crosspoint.type, crosspoint.i, crosspoint.j, crosspoint.score);
    push_back(crosspoint);
    fflush(file);
}

void CrosspointsFile::save() {
	open();
	for (iterator it=begin() ; it != end(); it++ ) {
	    fprintf(file, "%d,%d,%d,%d\n", it->type, it->i, it->j, it->score);
	    //fprintf(stderr, "%d,%d,%d,%d\n", it->type, it->i, it->j, it->score);
	}
	close();
}

void CrosspointsFile::reverse(int seq0_len, int seq1_len) {
	for (int i=0; i < this->size(); i++) {
		at(i) = at(i).reverse(seq0_len, seq1_len);
	}
	std::reverse(this->begin(),this->end());
}

