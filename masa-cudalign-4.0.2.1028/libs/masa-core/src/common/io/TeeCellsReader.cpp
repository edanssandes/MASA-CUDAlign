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

#include "TeeCellsReader.hpp"

#include <cstdio>
#include <cstdlib>

#define DEBUG (0)

TeeCellsReader::TeeCellsReader(CellsReader* reader, string filename) {
	this->reader = reader;
	this->filename = filename;

	this->fileWriter = new FileCellsWriter(filename);
	this->fileReader = NULL;
}

TeeCellsReader::~TeeCellsReader() {
	close();
}

void TeeCellsReader::close() {
	if (reader != NULL) {
		reader->close();
		reader = NULL;
	}
	if (fileWriter != NULL) {
		fileWriter->close();
		delete fileWriter;
		fileWriter = NULL;
	}
	if (fileReader != NULL) {
		fileReader->close();
		delete fileReader;
		fileReader = NULL;
	}
}

int TeeCellsReader::getType() {
	return INIT_WITH_CUSTOM_DATA;
}

int TeeCellsReader::read(cell_t* buf, int len) {
	if (reader != NULL) {
		int ret = reader->read(buf, len);
		fileWriter->write(buf, ret);
		return ret;
	} else if (fileReader != NULL){
		return fileReader->read(buf, len);
	} else {
		return -1;
	}
}

void TeeCellsReader::seek(int position) {
	if (DEBUG) printf("TeeCellsReader::seek(%d)\n", position);
	if (reader != NULL) {
		reader->close();
		reader = NULL;
	}
	if (fileWriter != NULL) {
		fileWriter->close();
		fileWriter = NULL;
	    fileReader = new FileCellsReader(filename);
	} else if (fileReader == NULL) {
		fprintf(stderr, "TeeCellsReader: Cannot seek.\n");
		exit(1);
	}
	this->fileReader->seek(position);
	if (DEBUG) printf("TeeCellsReader::seek() DONE\n");
}

int TeeCellsReader::getOffset() {
	return this->fileReader->getOffset();
}
