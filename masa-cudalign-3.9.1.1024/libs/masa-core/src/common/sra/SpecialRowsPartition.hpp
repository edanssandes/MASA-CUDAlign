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

class SpecialRowsPartition;

#ifndef SPECIALROWSPARTITION_HPP_
#define SPECIALROWSPARTITION_HPP_

#include "SpecialRow.hpp"

#include <string>
#include <map>
using namespace std;

#include "../../libmasa/libmasa.hpp"
#include "FirstRow.hpp"
#include "../io/CellsWriter.hpp"
#include "../io/SeekableCellsReader.hpp"

/**
 *
 */
class SpecialRowsPartition {
public:
	SpecialRowsPartition(string path, int i0, int j0, int i1, int j1, bool readOnly, const score_params_t* score_params);
	virtual ~SpecialRowsPartition();
	void reload();
	void deleteRows();

	void setRamProportion(const long long ram, const long long disk);

//	void setFirstRow(const score_params_t* score_params, bool firstRowGapped);
	void setFirstColumnReader(SeekableCellsReader* reader);
	void setFirstRowReader(SeekableCellsReader* reader);
	void setLastColumnWriter(CellsWriter* writer);
	void setLastRowWriter(CellsWriter* writer);

	int read(cell_t* buf, int len);
    int write(int i, const cell_t* buf, int len);
    int getLastRowId();
    SpecialRow* getLastRow();
    string getLastRowFilename();
    int getRowsCount() const;
    int getReadingRow();
    int getLargestInterval();

    SpecialRow* nextSpecialRow(int i, int j, int min_dist=0) ;
    void truncate(int max_i, int max_j);
    void changePath(string new_path);

	int getI0() const;
	int getI1() const;
	int getJ0() const;
	int getJ1() const;
	const string& getPath() const;

	CellsWriter* getFirstColumnWriter();
	SeekableCellsReader* getFirstColumnReader();
	SeekableCellsReader* getFirstRowReader();
	CellsWriter* getLastColumnWriter();
	CellsWriter* getLastRowWriter();

	void createChain(SpecialRowsPartition* otherPartition);

	string getFirstColumnFilename();
	string getFirstRowFilename();

	int continueFromLastRow();

	bool isPersistent() const;

private:
    int i0;
    int j0;
    int i1;
    int j1;

    long long ramProportion;
    long long diskProportion;
    int ramCount;
    int diskCount;
    const score_params_t* score_params;

    bool readOnly;
    bool persistent;
    string path;
    map<int, SpecialRow*> rowsMap;
    vector<SpecialRow*> rowsVector;
    int largestInterval;
    int readingRowId;
    int lastRowId;
    string lastRowFilename;
    //int readingRowOffset;
    SpecialRow* readingRow;
    FirstRow firstRow;
    CellsWriter* firstColumnWriter;
    CellsWriter* firstRowWriter;
    SeekableCellsReader* firstColumnReader;
    SeekableCellsReader* firstRowReader;
    CellsWriter* lastColumnWriter;
    CellsWriter* lastRowWriter;
	void updateLargestInterval();

	//pthread_mutex_t mutex;

    void readDirectory();
    SpecialRow* getSpecialRow(int i);
    void setBorderReader(char prefix, SeekableCellsReader* &reader, CellsWriter* &writer);
    bool loadBorderReader(char prefix, string file, SeekableCellsReader* &reader);

};

#endif /* SPECIALROWSPARTITION_HPP_ */
