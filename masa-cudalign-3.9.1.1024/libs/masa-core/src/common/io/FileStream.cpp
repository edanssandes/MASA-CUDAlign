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

#include "FileStream.hpp"


#include <stdlib.h>
#include <stdio.h>

FileStream::FileStream(string filename) {
	FILE* file = fopen(filename.c_str(), "a+b");
	if (file == NULL) {
		fprintf(stderr, "FileStream: Could not create file (%s).\n", filename.c_str());
		exit(1);
	}
	this->file = file;
	this->posRead = 0;
}

FileStream::~FileStream() {
	close();
}

void FileStream::close() {
	if (file != NULL) {
		fclose(file);
		file = NULL;
	}
}

int FileStream::getType() {
	return INIT_WITH_CUSTOM_DATA;
}

int FileStream::read(cell_t* buf, int len) {
	fseek(file, posRead*sizeof(cell_t), SEEK_SET);
	int ret = fread(buf, sizeof(cell_t), len, file);
	if (ret > 0) {
		posRead += ret;
	}
	return ret;
}

int FileStream::write(const cell_t* buf, int len) {
	return fwrite(buf, sizeof(cell_t), len, file);
}

void FileStream::seek(int position) {
	//fseek(file, position*sizeof(cell_t), SEEK_SET);
	posRead = position;
}

int FileStream::getOffset() {
	return posRead;
}
