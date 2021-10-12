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

#include "SpecialRowReader.hpp"

#include <string.h>

#include <algorithm>
using namespace std;

#include <dirent.h>
#include <stdlib.h>
#include "../libmasa/IAligner.hpp"

#define DEBUG (0)


SpecialRowReader::SpecialRowReader(string directory, const score_params_t* score_params, bool firstRowGapped) {
    this->firstRowGapped = firstRowGapped;
    this->directory = directory;
    this->score_params = score_params;
    loadSpecialRows();
    id = 0;
    file = NULL;

}

SpecialRowReader::~SpecialRowReader() {
    close();
}

int SpecialRowReader::getCol() const {
    return specialRowCoordinates[id].col;
}

int SpecialRowReader::getRow() const {
    return specialRowCoordinates[id].row;
}

string SpecialRowReader::getFilename(int row, int col) {
	char str[500];
	if (row == -1) {
		row = getRow();
	}
	if (col == -1) {
		col = getCol();
	}
	sprintf(str, "%s/%08X.%08X", directory.c_str(), row, col);
	return string(str);
}

void SpecialRowReader::open(int start) {
	close();
    this->start = start;

    string filename = getFilename();
    if (DEBUG) printf("SpecialRow: %s (%d)\n", filename.c_str(), start);
    this->file = fopen(filename.c_str(), "rb");
    printf("%s\n", filename.c_str());
    if (getRow() == 0 && getCol() == 0) {
    	printf("First Special Row (0,0)\n");
    } else if (file == NULL) {
        fprintf(stderr, "Could not open special row file: %s\n", filename.c_str());
        exit(1);
    }
}

void SpecialRowReader::close() {
    if (file != NULL) {
        printf("File Close: %p\n", file);
        fclose(file);
        file = NULL;
    }
}

int SpecialRowReader::getCurrentPosition() {
    return current;
}

int SpecialRowReader::read(cell_t* buf, int offset, int len) {
	int revOffset = start-offset-(len-1);
	if (revOffset < 0) {
		revOffset = 0;
		len = start-offset+1;
		if (len < 0) len = 0;
	}
	if (file == NULL) {
		if (score_params == NULL) {
			fprintf(stderr, "No score parameters supplied for the first row.\n");
			exit(1);
		}
		for (int k=0; k<len; k++) {
			int ir = revOffset+(len-1-k);
			if (firstRowGapped) {
				buf[k].h = (ir==0) ? 0 : -ir*score_params->gap_ext-score_params->gap_open;
			} else {
				buf[k].h = 0;
			}
			buf[k].f = -INF;
		}
	} else {
		int r=0;
		fseek(file, revOffset*sizeof(cell_t), SEEK_SET);
		if (DEBUG) printf("READ [%d..%d] (%d) POS_SEEK: %ld\n", revOffset, revOffset+len-1,  len, ftell(file));
		while (r<len) {
			int ret = fread(buf, sizeof(cell_t), len-r, file);
			if (ret == 0) {
				fprintf(stderr, "Error: End of special row (%d).\n", len-r);
				exit(1);
			}
			r += ret;
		}
		for (int i=0; i<len/2; i++) {
			cell_t aux = buf[i];
			buf[i] = buf[len-1-i];
			buf[len-1-i] = aux;
		}
	}

    return len;
}

int coordinate_t::sortf(coordinate_t a, coordinate_t b) {
    if (a.row != b.row) {
        return a.row > b.row;
    } else {
        return a.col > b.col;
    }
}

void SpecialRowReader::loadSpecialRows() {
    DIR *dir = NULL;

    dir = opendir (directory.c_str());
    struct dirent *dp;          /* returned from readdir() */

    if (dir == NULL) {
    	fprintf(stderr, "Could not open special rows directory: %s\n", directory.c_str());
        exit(1);
    }

    while ((dp = readdir (dir)) != NULL) {
    	coordinate_t coords;
        int col;
        if (sscanf(dp->d_name, "%X.%X", &coords.row, &coords.col) == 2) {
            specialRowCoordinates.push_back(coords);
        }
    }
    coordinate_t coords = {0,0};
    specialRowCoordinates.push_back(coords);

    closedir (dir);
    sort(specialRowCoordinates.begin(), specialRowCoordinates.end(), coordinate_t::sortf);

    if (DEBUG) {
		vector<coordinate_t>::iterator it;
		for (it=specialRowCoordinates.begin() ; it < specialRowCoordinates.end(); it++) {
			printf ("(%08X.%08X)\n", (*it).row, (*it).col);
		}
    }
}


bool SpecialRowReader::nextSpecialRow(int row, int col, int min_dist) {
	close();

    int count = specialRowCoordinates.size();

	while (id<count && specialRowCoordinates[id].row > row - min_dist) {
		if (DEBUG) printf("l: %d  (%d,%d) - (%d,%d)\n", id,
				specialRowCoordinates[id].row, specialRowCoordinates[id].col,
				row, col);
		id++;
	}

    printf("id: %d\n", id);
    if (id >= count) {
    	id = count-1; // coordinate {0,0}
        printf("End of Special Lines\n");
        return false;
    }

    printf("-[%d.%d]\n", row, col);
    row = specialRowCoordinates[id].row;
    col = specialRowCoordinates[id].col;

    printf("+[%d.%d]\n", row, col);

    return true;
}

int SpecialRowReader::getLargestInterval(int* max_i, int* max_j) {

	int max_size_i = 0;
	int max_size_j = 0;
	for (int i=1; i < specialRowCoordinates.size(); i++) {
		coordinate_t curr = specialRowCoordinates[i];
		coordinate_t prev = specialRowCoordinates[i-1];
		int delta_i = prev.row - curr.row;
		int delta_j = prev.col - curr.col;
		if (delta_i !=0 && delta_j != 0) {
			// TODO conferir. Principalmente se estiver no stage 3!! pois teriamos que contar o crosspoint.
			// TODO talvez devessemos salvar um special row dummy pra evitar isso.
			continue;
		}
		if (max_size_i < delta_i) max_size_i = delta_i;
		if (max_size_j < delta_j) max_size_j = delta_j;
	}
	if (max_i != NULL) *max_i = max_size_i;
	if (max_j != NULL) *max_j = max_size_j;
	if (max_size_i > max_size_j) {
		return max_size_i;
	} else {
		return max_size_j;
	}
}
