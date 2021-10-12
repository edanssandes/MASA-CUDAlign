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

#include "FileCellsReader.hpp"


#include <stdlib.h>

FileCellsReader::FileCellsReader(FILE* file) {
	this->file = file;
}

FileCellsReader::FileCellsReader(const string path) {
	FILE* file = fopen(path.c_str(), "rb");
	if (file == NULL) {
		fprintf(stderr, "Could not open file (%s).\n", path.c_str());
		exit(1);
	}
	this->file = file;
}

FileCellsReader::~FileCellsReader() {
	close();
}

void FileCellsReader::close() {
	if (file != NULL) {
		fclose(file);
		file = NULL;
	}
}

int FileCellsReader::getType() {
	return INIT_WITH_CUSTOM_DATA;
}

int FileCellsReader::read(cell_t* buffer, int len) {
	int p = 0;
	if (buffer == NULL) {
		fseek(file, len, SEEK_CUR);
	} else {
		while (p < len) {
			int ret = fread(buffer, sizeof(cell_t), len-p, file);
			if (ret <= 0) {
				if (feof(file)) {
					fprintf(stderr, "FileCellsReader::read(): End-of-file.\n");
					return p;
				} else {
					fprintf(stderr, "FileCellsReader::read() - failed to read completely [%d/%d]\n", p, len-p);
					fprintf(stderr, "Could not load busH file (%d).\n", ret);
					exit(1);
				}
			}
			p += ret;
		}
	}
	return len;
}

void FileCellsReader::seek(int position) {
	fseek(file, position*sizeof(cell_t), SEEK_SET);
}

int FileCellsReader::getOffset() {
	return ftell(file)/sizeof(cell_t);
}
