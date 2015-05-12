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

#include "SpecialRowsPartition.hpp"
#include "SpecialRowFile.hpp"
#include "SpecialRowRAM.hpp"
#include "FirstRow.hpp"
#include "../io/FileCellsWriter.hpp"
#include "../io/FileCellsReader.hpp"
#include "../io/InitialCellsReader.hpp"
#include "../io/TeeCellsReader.hpp"
#include "../io/FileStream.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>

#include <algorithm>
using namespace std;

#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#define DEBUG	(0)

#define def2str(x) #x

SpecialRowsPartition::SpecialRowsPartition(string _path, int _i0, int _j0, int _i1, int _j1, bool _readOnly, const score_params_t* score_params)
        : path(_path), i0(_i0), j0(_j0), i1(_i1), j1(_j1), readOnly(_readOnly) {

	this->readingRow = NULL;
	this->ramProportion = 1;
	this->diskProportion = 0;
	this->ramCount = 0;
	this->diskCount = 0;
	this->lastRowId = 0;
	this->lastRowFilename = "";
	this->firstColumnWriter = NULL;
	this->score_params = score_params;

    rowsVector.push_back(&firstRow);
	//this->path = getPartitionPath(i0, j0, i1, j1);
    if (readOnly) {
    	persistent = true;
    	struct stat s;
    	if (stat(path.c_str(), &s) == -1) {
    	    if(errno == ENOENT) {
    			fprintf(stderr, "Path (%s) does not exist. (errno: %d)\n", path.c_str(), errno);
    			exit(1);
    	    }
    	}
    } else {
    	persistent = (path.length() != 0);
    	if (persistent && mkdir(path.c_str(), 0774)) {
    		if (errno != EEXIST) {
    			fprintf(stderr, "Path (%s) could not be created. Try to create it manually. (errno: %d)\n", path.c_str(), errno);
    			exit(1);
    		}
    	}
    }

    if (persistent) {
    	readDirectory();
    }
}

SpecialRowsPartition::~SpecialRowsPartition() {
    if (readingRow != NULL) {
    	readingRow->close();
    }
	//truncate(i1, j1); // TODO podemos retirar definitivamente?
    for (map<int, SpecialRow*>::iterator it = rowsMap.begin(); it != rowsMap.end(); it++) {
        SpecialRow* row = (*it).second;
       	delete row;
    }
    rowsMap.clear();
	for (vector<SpecialRow*>::iterator it = rowsVector.begin(); it != rowsVector.end(); it++) {
		SpecialRow* row = (*it);
		if (row != &firstRow) {
			delete row;
		}
	}
	rowsVector.clear();
}

//void SpecialRowsPartition::setFirstRow(const score_params_t* score_params, 	bool firstRowGapped) {
//	firstRow.setParams(score_params, firstRowGapped);
//}

void SpecialRowsPartition::setFirstColumnReader(SeekableCellsReader* reader) {
	setBorderReader('C', reader, firstColumnWriter);
	this->firstColumnReader = reader;
}

void SpecialRowsPartition::setFirstRowReader(SeekableCellsReader* reader) {
	setBorderReader('R', reader, firstRowWriter);
	this->firstRowReader = reader;
}


void SpecialRowsPartition::setLastColumnWriter(CellsWriter* writer) {
	this->lastColumnWriter = writer;
}

void SpecialRowsPartition::setLastRowWriter(CellsWriter* writer) {
	this->lastRowWriter = writer;
}

void SpecialRowsPartition::setBorderReader(char prefix, SeekableCellsReader* &reader,
		CellsWriter* &writer) {
	if (reader == NULL) return;
	if (!persistent) return;

	char str[20];
	int startOffset;
	if (reader->getType() == INIT_WITH_CUSTOM_DATA) {
		startOffset = 0;
	} else {
		InitialCellsReader* initial = (InitialCellsReader*)reader;
		startOffset = initial->getStartOffset();
		//startOffset = -reader->getOffset();
	}
	sprintf(str, "%c%08X", prefix, startOffset);
	stringstream filename;
	filename << path << "/" << str << ".";

	switch (reader->getType()) {
	case INIT_WITH_CUSTOM_DATA:
		filename << def2str(INIT_WITH_CUSTOM_DATA);
		break;
	case INIT_WITH_ZEROES:
		filename << def2str(INIT_WITH_ZEROES);
		break;
	case INIT_WITH_GAPS:
		filename << def2str(INIT_WITH_GAPS);
		break;
	case INIT_WITH_GAPS_OPENED:
		filename << def2str(INIT_WITH_GAPS_OPENED);
		break;
	}

	if (reader->getType() == INIT_WITH_CUSTOM_DATA) {
		//writer = new FileCellsWriter(filename.str());
		reader = new TeeCellsReader(reader, filename.str());
		//TODO
	} else {

		FILE* file = fopen(filename.str().c_str(), "wb");
		if (file == NULL) {
			fprintf(stderr, "Could not create file (%s).\n", filename.str().c_str());
			exit(1);
		}
		fclose(file);
	}
}

void SpecialRowsPartition::reload() {
    sort(rowsVector.begin(), rowsVector.end(), SpecialRow::sortById);
	readingRowId = rowsVector.size() - 1;
	readingRow = NULL;
    lastRowId = rowsVector.back()->getId();

	updateLargestInterval();
}

int SpecialRowsPartition::getLastRowId() {
	return i0 + lastRowId;
}


SpecialRow* SpecialRowsPartition::getLastRow() {
	return rowsVector.back();
}

string SpecialRowsPartition::getLastRowFilename() {
	return lastRowFilename;
}

int SpecialRowsPartition::getReadingRow() {
	return i0 + readingRow->getId();
}

void SpecialRowsPartition::truncate(int max_i, int max_j) {
    if (DEBUG) printf("Flush: %08X,%08X\n", max_i, max_j);
	for (vector<SpecialRow*>::iterator it = rowsVector.begin(); it != rowsVector.end(); ) {
		SpecialRow* row = (*it);
        if ((row->getId() + i0) >= max_i && row != &firstRow) {
        	it = rowsVector.erase(it);
        	row->close();
        	row->truncateRow(0);
        	// precaution: the first row cannot be deleted.
        	delete row;
        } else {
        	row->truncateRow(max_j-j0+1);
        	it++;
        }
	}

    for (map<int, SpecialRow*>::iterator it = rowsMap.begin(); it != rowsMap.end(); it++) {
        SpecialRow* row = (*it).second;
       	row->close();
        if ((row->getId() + i0) >= max_i) {
        	row->truncateRow(0);
        	delete row;
        } else {
        	row->truncateRow(max_j-j0+1);
        	rowsVector.push_back(row);
        }
    }
    rowsMap.clear();
    i1 = max_i;
    j1 = max_j;
    lastRowId = rowsVector.back()->getId();

    updateLargestInterval();
}

/*void SpecialRowsPartition::translate(int i0, int j0, int i1, int j1) {
	rename(getPartitionPath(this->i0, this->j0, this->i1, this->j1).c_str(),
			getPartitionPath(i0, j0, i1, j1).c_str());
	this->i0 = i0;
	this->i1 = i1;
	this->j0 = j0;
	this->j1 = j1;
}*/

int SpecialRowsPartition::getLargestInterval() {
	return largestInterval;
}

int SpecialRowsPartition::getI0() const {
	return i0;
}

int SpecialRowsPartition::getI1() const {
	return i1;
}

int SpecialRowsPartition::getJ0() const {
	return j0;
}

int SpecialRowsPartition::getJ1() const {
	return j1;
}

const string& SpecialRowsPartition::getPath() const {
	return path;
}

void SpecialRowsPartition::changePath(string new_path) {
	rename(path.c_str(), new_path.c_str());
	path = new_path;
}

void SpecialRowsPartition::setRamProportion(const long long ram, const long long disk) {
	this->ramProportion = ram;
	this->diskProportion = disk;
}

int SpecialRowsPartition::getRowsCount() const {
	return rowsVector.size();
}

string SpecialRowsPartition::getFirstColumnFilename() {
	return path + "/" + "C00000000." + def2str(INIT_WITH_CUSTOM_DATA);
}

string SpecialRowsPartition::getFirstRowFilename() {
	return path + "/" + "R00000000." + def2str(INIT_WITH_CUSTOM_DATA);
}

CellsWriter* SpecialRowsPartition::getFirstColumnWriter() {
	if (firstColumnWriter == NULL) {
		firstColumnWriter = new FileCellsWriter(getFirstColumnFilename());
	}
	return firstColumnWriter;
}

SeekableCellsReader* SpecialRowsPartition::getFirstColumnReader() {
	return firstColumnReader;
}

SeekableCellsReader* SpecialRowsPartition::getFirstRowReader() {
	return firstRowReader;
}

CellsWriter* SpecialRowsPartition::getLastColumnWriter() {
	return lastColumnWriter;
}

CellsWriter* SpecialRowsPartition::getLastRowWriter() {
	return lastRowWriter;
}

SpecialRow* SpecialRowsPartition::getSpecialRow(int i) {
	SpecialRow* row = NULL;
	row = rowsMap[i];
	if (row == NULL && persistent) {
		// Alternate the creation of rows in disk and in ram.
		if ((diskProportion !=0 && ramProportion==0) || ramCount*diskProportion > ramProportion*diskCount) {
			row = new SpecialRowFile(&path, i);
			diskCount++;
		} else {
			row = new SpecialRowRAM(i);
			ramCount++;
		}

    	row->open(readOnly, j1-j0+1);
		rowsMap[i] = row;
	}
	return row;
}

int SpecialRowsPartition::write(int i, const cell_t* buf, int len) {
	if (readOnly) {
    	fprintf(stderr, "Fatal: Writing into a read-only SRA Partition");
    	exit(1);
	}
	if (!persistent) {
		return 0;
	}
	SpecialRow* row = getSpecialRow(i-i0);
	int ret = row->write(buf, len);
	if (row->getOffset() >= (j1-j0)+1) {
		row->close();
		rowsMap.erase(i-i0);
		rowsVector.push_back(row);
		lastRowId = row->getId();
	}

	return ret;
}

void SpecialRowsPartition::updateLargestInterval() {
	largestInterval = 0;
	for (int i = 1; i < rowsVector.size(); i++) {
		int diff = rowsVector[i]->getId() - rowsVector[i-1]->getId();
		if (largestInterval < diff) {
			largestInterval = diff;
		}
	}
}

void SpecialRowsPartition::readDirectory() {
	// This method insert new rows to the vectors.
    DIR *dir = NULL;
    //printf("Opening Dir: %s\n", path.c_str());
    dir = opendir (path.c_str());
    struct dirent *dp;          /* returned from readdir() */

    if (dir == NULL) {
    	fprintf(stderr, "Could not open special rows directory: %s\n", path.c_str());
        exit(1);
    }

    while ((dp = readdir (dir)) != NULL) {
        int id;
        if (loadBorderReader('C', string(dp->d_name), firstColumnReader)) {
        	continue;
        }
        if (loadBorderReader('R', string(dp->d_name), firstRowReader)) {
        	firstRow.setCellsReader(firstRowReader);
        	continue;
        }
		SpecialRow* row = new SpecialRowFile(&path, string(dp->d_name));
		if (row->getId() < 0) {
			delete row;
		} else {
			string rowFilename = path + "/" + string(dp->d_name);
			//fprintf(stderr, "Loading... %s: %d\n", rowFilename.c_str(), row->getId());
			rowsVector.push_back(row);
			if (row->getId() > lastRowId) {
				lastRowId = row->getId();
				lastRowFilename = rowFilename;
			}
		}

    }
    closedir (dir);

    reload();
}

SpecialRow* SpecialRowsPartition::nextSpecialRow(int i, int j, int min_dist) {
	//close();
    int count = rowsVector.size();
    if (count == 0) {
    	return NULL;
    }

    while (readingRowId >= 0) {
    	int dist = ((i-i0)-rowsVector[readingRowId]->getId());
		if (DEBUG) printf("l: %d  %d %d (%d, %d)\n", readingRowId, rowsVector[readingRowId]->getId(), dist, i,j);
    	if (readingRowId == 0) {
    		if (dist > 0) {
    			// Ensure that the row with id=0 is processed.
    			break;
    		} else {
    	        printf("End of Special Lines\n");
    	        return NULL;
    		}
    	} else {
    		if (dist > min_dist) {
    			break;
    		} else {
    			readingRowId--;
    		}
    	}
	};
    if (readingRow != NULL) {
    	readingRow->close();
    }

	readingRow = rowsVector[readingRowId];
	if (DEBUG) printf("id: %d %s/%08X\n", readingRowId, path.c_str(), readingRow->getId());

	if (readingRow == &firstRow) {
		firstRow.setCellsReader(firstRowReader);
	}

	readingRow->open(true);
	int readingRowOffset = abs(j-j0)+1;
	readingRow->seek(readingRowOffset);

    return readingRow;
}

int SpecialRowsPartition::read(cell_t* buf, int len) {
	int ret = readingRow->read(buf, len);
	return ret;
}

int SpecialRowsPartition::continueFromLastRow() {
	firstColumnReader->read(NULL, lastRowId);
	// TODO o first row source precisa considerar a linha onde ele se encontra (i.e. primeira celula possui valor != 0)
	// TODO Lembrando que eu retirei o primeiro dispatch cell.
	setFirstRowReader(new FileCellsReader(lastRowFilename.c_str()));
	printf("Continuing partition from row %d (%s)\n",
			lastRowId + i0, lastRowFilename.c_str());
	return lastRowId + i0;
}

bool SpecialRowsPartition::isPersistent() const {
	return persistent;
}

void SpecialRowsPartition::createChain(SpecialRowsPartition* that) {
	if (that->getI1() == this->getI0()) {
		FileStream* stream = new FileStream(this->getFirstRowFilename());
		that->lastRowWriter = stream;
		this->firstRowReader = stream;
	} else if (that->getJ1() == this->getJ0()) {
		FileStream* stream = new FileStream(this->getFirstColumnFilename());
		that->lastColumnWriter = stream;
		this->firstColumnReader = stream;
	} else if (this->getI1() == that->getI0()) {
		FileStream* stream = new FileStream(that->getFirstRowFilename());
		this->lastRowWriter = stream;
		that->firstRowReader = stream;
	} else if (this->getJ1() == that->getJ0()) {
		FileStream* stream = new FileStream(that->getFirstColumnFilename());
		this->lastColumnWriter = stream;
		that->firstColumnReader = stream;
	}

}

void SpecialRowsPartition::deleteRows() {
	truncate(-1, -1);
}

bool SpecialRowsPartition::loadBorderReader(char prefix, string file, SeekableCellsReader* &reader) {
    if (file[0] == prefix) {
    	string type = file.substr(10);
    	int offset;
    	sscanf(file.c_str(), "%*c%08X", &offset);
    	if (type == def2str(INIT_WITH_CUSTOM_DATA)) {
    		reader = new FileCellsReader(path + "/" + file);
    	} else if (type == def2str(INIT_WITH_ZEROES)) {
    		reader = new InitialCellsReader(offset);
    	} else if (type == def2str(INIT_WITH_GAPS)) {
    		// TODO is this correct? FIXME gapOpen/gapFirst ?
    		reader = new InitialCellsReader(score_params->gap_open + offset*score_params->gap_ext, score_params->gap_ext);
    	} else if (type == def2str(INIT_WITH_GAPS_OPENED)) {
    		// TODO is this correct? FIXME gapOpen/gapFirst ?
    		reader = new InitialCellsReader(offset*score_params->gap_ext, score_params->gap_ext);
    	} else {
    		return false;
    	}
    	return true;
    }
    return false;
}

