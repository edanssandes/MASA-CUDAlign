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

#include "SpecialRowsArea.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <algorithm>

#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

#include "../io/SplitCellsReader.hpp"
#include "../io/InitialCellsReader.hpp"

#define DEBUG (0)

SpecialRowsArea::SpecialRowsArea(string directory, long long ram_limit, long long disk_limit, const score_params_t* score_params) {
	this->directory = directory;
	this->ram_limit = ram_limit;
	this->disk_limit = disk_limit;
	//printf("SpecialRowsArea::SpecialRowsArea   directory: %s\n", directory.c_str());
	this->partitions.clear();
	this->rowsCount = 0;
	this->score_params = score_params;
	this->persistentPartitions = true;//(ram_limit+disk_limit) > 0; // FIXME or TODO
}

SpecialRowsArea::~SpecialRowsArea() {
	for (map<string, SpecialRowsPartition*>::const_iterator i = partitions.begin(); i != partitions.end(); i++) {
		delete i->second;
	}
	partitions.clear();
}

SpecialRowsPartition* SpecialRowsArea::createPartition(int i0, int j0, int i1, int j1) {
	string path = getPartitionPath(i0, j0, i1, j1);
	SpecialRowsPartition* partition = new SpecialRowsPartition(path, i0, j0, i1, j1, false, score_params);
	partition->setRamProportion(ram_limit, disk_limit);

	partitions[path] = partition;
	return partition;
}

SpecialRowsPartition* SpecialRowsArea::openPartition(int i0, int j0, int i1, int j1) {
	string path = getPartitionPath(i0, j0, i1, j1);
	if (partitions[path] == NULL) {
		SpecialRowsPartition* partition = new SpecialRowsPartition(path, i0, j0, i1, j1, true, score_params);
		partitions[path] = partition;
		return partition;
	} else {
		partitions[path]->reload();
	}

	//printf("SpecialRowsArea::openPartition   path: %s  %p\n", path.c_str(), partitions[path]);
	return partitions[path];
}

void SpecialRowsArea::truncatePartition(SpecialRowsPartition* partition, int max_i, int max_j) {
	partition->truncate(max_i, max_j);

	string old_path = partition->getPath();
	string new_path = getPartitionPath(partition->getI0(), partition->getJ0(), max_i, max_j);
	partition->changePath(new_path);

	partitions.erase(old_path);
	partitions[new_path] = partition;

	rowsCount += partition->getRowsCount();
	/*for (map<string, SpecialRowsPartition*>::const_iterator i = partitions.begin(); i != partitions.end(); i++) {
		printf("+map %s->%p\n", i->first.c_str(), i->second);
	}*/
}

int SpecialRowsArea::getRowsCount() const {
	if (!persistentPartitions) {
		return 0;
	}
	// Only works for newly created areas.
	return rowsCount;
}

int SpecialRowsArea::getPartitionsCount() const {
	// Only works for newly created areas.
	return partitions.size();
}

const string& SpecialRowsArea::getDirectory() const {
	return directory;
}

void SpecialRowsArea::setPersistentPartitions(bool persistent) {
	this->persistentPartitions = persistent;
}

SpecialRowsPartition* SpecialRowsArea::openPartition(int i, int j) {
	DIR *dir = NULL;
	//printf("Opening Dir: %s\n", path.c_str());
	dir = opendir (directory.c_str());
	struct dirent *dp;          /* returned from readdir() */

	if (dir == NULL) {
		fprintf(stderr, "Could not open special rows directory: %s\n", directory.c_str());
		exit(1);
	}

	while ((dp = readdir (dir)) != NULL) {
		int i0;
		int j0;
		int i1;
		int j1;
		if (sscanf(dp->d_name, "%08X.%08X.%08X.%08X", &i0, &j0, &i1, &j1) == 4) {
			if (i0 < i && i <= i1 && j0 < j && j <= j1) {
				closedir (dir);
				if (DEBUG) printf("DEBUG: OPEN >>>>>> %s (%d,%d,%d,%d)\n", dp->d_name, i0, j0, i1, j1);
				return openPartition(i0, j0, i1, j1);
			}
		}
		if (DEBUG) printf("%d %d %d %d\n", i0, j0, i1, j1);
	}
	closedir (dir);

	return NULL;
}

void SpecialRowsArea::createSplittedPartitions(int i0, int j0, int i1, int j1, int ni, int nj, SeekableCellsReader* firstRow, SeekableCellsReader* firstColumn, CellsWriter* lastRow, CellsWriter* lastColumn) {
//	SeekableCellsReader* firstRow = partition->getFirstRowReader();
//	SeekableCellsReader* firstColumn = partition->getFirstColumnReader();
//	CellsWriter* lastRow = partition->getLastRowWriter();
//	CellsWriter* lastColumn = partition->getLastColumnWriter();

	long long int h = (i1 - i0);
	long long int w = (j1 - j0);

	SpecialRowsPartition*** p = new SpecialRowsPartition**[ni];
	for (int pi=0; pi<ni; pi++) {
		p[pi] = new SpecialRowsPartition*[nj];
		for (int pj=0; pj<nj; pj++) {

			int pi0 = (int)(((long long int)i0) + h*pi/ni);
			int pj0 = (int)(((long long int)j0) + w*pj/nj);
			int pi1 = (int)(((long long int)i0) + h*(pi+1)/ni);
			int pj1 = (int)(((long long int)j0) + w*(pj+1)/nj);
			printf(">>>>>>>>>> %d %d %d %d   %d\n", pi0, pj0, pi1, pj1, firstRow->getOffset());

			p[pi][pj] = createPartition(pi0, pj0, pi1, pj1);
			if (pi == ni-1) {
				p[pi][pj]->setLastRowWriter(lastRow);
			}
			if (pj == nj-1) {
				p[pi][pj]->setLastColumnWriter(lastColumn);
			}

			if (pi == 0 && pj == 0) {
				p[pi][pj]->setFirstColumnReader(firstColumn);
				p[pi][pj]->setFirstRowReader(firstRow);
			} else {
				if (pi == 0) {
					if (firstRow != NULL && (firstRow->getType() != INIT_WITH_CUSTOM_DATA)) {
						p[pi][pj]->setFirstRowReader(((InitialCellsReader*)firstRow)->clone(pj0-j0));
					} else {
						p[pi][pj]->setFirstRowReader(firstRow);
					}
				}
				if (pj == 0) {
					if (firstColumn != NULL && (firstColumn->getType() != INIT_WITH_CUSTOM_DATA)) {
						p[pi][pj]->setFirstColumnReader(((InitialCellsReader*)firstColumn)->clone(pi0-i0));
					} else {
						p[pi][pj]->setFirstColumnReader(firstColumn);
					}
				}
//				if (pi == 0 && firstRow != NULL) {
//					p[pi][pj]->setFirstRowReader(new SplitCellsReader(firstRow, pj0, pj==nj-1));
//				}
//				if (pj == 0 && firstColumn != NULL) {
//					p[pi][pj]->setFirstColumnReader(new SplitCellsReader(firstColumn, pi0, pi==ni-1));
//				}
			}
			if (pi != 0) {
				p[pi-1][pj]->createChain(p[pi][pj]);
			}
			if (pj != 0) {
				p[pi][pj-1]->createChain(p[pi][pj]);
			}
		}
	}
}

string SpecialRowsArea::getPartitionPath(int i0, int j0, int i1, int j1) {
	if (!persistentPartitions) {
		return "";
	}
	char str[500];
	sprintf(str, "%s/%08X.%08X.%08X.%08X", directory.c_str(), i0, j0, i1, j1);
	return string(str);
}


static bool sort_f(const SpecialRowsPartition* a, const SpecialRowsPartition* b) {
	int deltaI = a->getI0() - b->getI0();
	int deltaJ = a->getJ0() - b->getJ0();
	if (deltaI == 0) {
		return deltaJ < 0;
	} else {
		return deltaI < 0;
	}
}

vector<SpecialRowsPartition*> SpecialRowsArea::getSortedPartitions() {
	vector<SpecialRowsPartition*> sortedPartitions;
	for (map<string, SpecialRowsPartition*>::iterator it = partitions.begin(); it != partitions.end(); ++it) {
		sortedPartitions.push_back(it->second);
	}
	std::sort (sortedPartitions.begin(), sortedPartitions.end(), sort_f);
	vector<int> openY;
	return sortedPartitions;
}


