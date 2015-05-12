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

#ifndef _SPECIALLINE_HPP
#define	_SPECIALLINE_HPP

#include <stdio.h>
#include <string>
#include <vector>
using namespace std;

#include "../libmasa/libmasa.hpp"

struct coordinate_t {
	int col;
	int row;

	static int sortf(coordinate_t a, coordinate_t b);
};

class SpecialRowReader {
public:
    SpecialRowReader(string directory, const score_params_t* score_params, bool firstRowGapped = false);
    virtual ~SpecialRowReader();

    int getCol() const;
    int getRow() const;

    void open(int start);
    void close();
    int read(cell_t* buf, int offset, int len); //TODO desvincular cell_t
    int getCurrentPosition();
    string getFilename(int row=-1, int col=-1);

    bool nextSpecialRow(int row, int col, int min_dist);
    int getLargestInterval(int* max_i, int* max_j);

private:
    void loadSpecialRows();
    int j0;
    int j1;
    string directory;
    const score_params_t* score_params;
    FILE* file;
    vector<coordinate_t> specialRowCoordinates;
    int id;

    int start;
    int min_step;
    int current;
    int firstRowGapped;
};

#endif	/* _SPECIALLINE_HPP */

