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

#include "BlocksFile.hpp"

#include <stdio.h>
#include <stdlib.h>

BlocksFile::BlocksFile(string filename) {
	this->filename = filename;
	this->file = NULL;
	this->initialized = false;
	this->buffer = NULL;
}

BlocksFile::~BlocksFile()
{
	close();
	if (buffer != NULL) {
		delete buffer;
		buffer = NULL;
	}
	initialized = false;
}

void BlocksFile::initialize(const Grid* grid) {
	file = fopen(filename.c_str(), "wb");
	if (file == NULL) {
		fprintf(stderr, "Could not create block file: %s\n", filename.c_str());
		exit(1);
	}
	this->width = grid->getGridWidth();
	this->height = grid->getGridHeight();
	fwrite(&height, sizeof(int), 1, file);
	fwrite(&width, sizeof(int), 1, file);
	initialized = true;
}

void BlocksFile::setScore(int bx, int by, int score) {
	//if (bl < cutBlockX || bl >= cutBlockY)
	//	w = 0x80000000; // MIN_INT
	if (by >= 0 && by < height && bx >= 0 && bx < width) {
		int pos = by * width + bx;
		fseek(file, (pos + 2) * sizeof(int), SEEK_SET);
		fwrite(&score, sizeof(int), 1, file);
	}
}

void BlocksFile::close() {
	if (file != NULL) {
		fclose(file);
		file = NULL;
		initialized = false;
	}
}

bool BlocksFile::isInitialized() {
	return initialized;
}

int* BlocksFile::reduceData(int &bh, int &bw) {
	FILE* file = fopen(filename.c_str(), "rb");
	if (file == NULL) {
		fprintf(stderr, "Could not open block file: %s\n", filename.c_str());
		exit(1);
	}

	int h;
	int w;
	fread(&h, sizeof(int), 1, file);
	fread(&w, sizeof(int), 1, file);

	if (bh > h) {
		bh = h;
	}
	if (bw > w) {
		bw = w;
	}

	if (buffer != NULL) {
		delete buffer;
	}
	buffer = new int[bh*bw];
	for (int bi=0; bi<bh; bi++) {
		for (int bj=0; bj<bw; bj++) {
			buffer[bi*bw + bj] = 0x80000000;
		}
	}


	for (int i=0; i<h; i++) {
		for (int j=0; j<w; j++) {
			int score;
			fread(&score, sizeof(int), 1, file);

			int bi = i*bh/h;
			int bj = j*bw/w;
			int index = bi*bw + bj;

			if (buffer[index] < score) {
				buffer[index] = score;
			}
		}
	}
	fclose(file);
	return buffer;
}
