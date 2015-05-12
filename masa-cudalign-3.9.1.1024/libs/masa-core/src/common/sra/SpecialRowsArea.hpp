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

class SpecialRowsArea;

#ifndef SPECIALROWSAREA_HPP_
#define SPECIALROWSAREA_HPP_

#include "SpecialRowsPartition.hpp"
#include "../../libmasa/libmasa.hpp"

#include <string>
#include <map>
#include <vector>
using namespace std;

class SpecialRowsArea {
public:
	SpecialRowsArea(string directory, long long ram_limit, long long disk_limit, const score_params_t* score_params);
	virtual ~SpecialRowsArea();

	SpecialRowsPartition* createPartition(int i0, int j0, int i1, int j1);
	SpecialRowsPartition* openPartition(int i0, int j0);
	SpecialRowsPartition* openPartition(int i0, int j0, int i1, int j1);
	void truncatePartition(SpecialRowsPartition* partition, int max_i, int max_j);
	void createSplittedPartitions(int i0, int j0, int i1, int j1, int ni, int nj, SeekableCellsReader* firstRow, SeekableCellsReader* firstColumn, CellsWriter* lastRow, CellsWriter* lastColumn);
	int getRowsCount() const;
	int getPartitionsCount() const;
	const string& getDirectory() const;
	void setPersistentPartitions(bool persistent);

	vector<SpecialRowsPartition*> getSortedPartitions();

private:

	bool persistentPartitions;
	string directory;
	long long disk_limit;
	long long ram_limit;
	map<string, SpecialRowsPartition*> partitions;
	int rowsCount;
	const score_params_t* score_params;

	string getPartitionPath(int i0, int j0, int i1, int j1);
};

#endif /* SPECIALROWSAREA_HPP_ */
