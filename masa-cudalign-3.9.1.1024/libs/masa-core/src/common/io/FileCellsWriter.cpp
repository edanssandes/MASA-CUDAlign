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

#include "FileCellsWriter.hpp"

#include <stdlib.h>

FileCellsWriter::FileCellsWriter(FILE* file) {
		this->file = file;
}

FileCellsWriter::FileCellsWriter(const string path) {
	FILE* file = fopen(path.c_str(), "wb");
	if (file == NULL) {
		fprintf(stderr, "FileCellsWriter: Could not create writer for file (%s).\n", path.c_str());
		exit(1);
	}
	this->file = file;
}

FileCellsWriter::~FileCellsWriter() {
	close();
}

void FileCellsWriter::close() {
	if (file != NULL) {
		fclose(file);
		file = NULL;
	}
}

int FileCellsWriter::write(const cell_t* buf, int len) {
	return fwrite(buf, sizeof(cell_t), len, file);
}
