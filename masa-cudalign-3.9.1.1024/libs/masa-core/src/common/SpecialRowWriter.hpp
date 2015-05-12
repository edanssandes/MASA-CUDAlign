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

#ifndef _SPECIALROWWRITER_HPP
#define	_SPECIALROWWRITER_HPP

#include <stdio.h>
#include <pthread.h>
#include <string>
#include <map>
using namespace std;

class SpecialRowWriter {
public:
    SpecialRowWriter(string directory, int i0, int j0, int j1);
    virtual ~SpecialRowWriter();

    int write(int i, void* buf, int len);
    int getLastRow();

    void flush(int min_i=-1);
private:
    int i0;
    int j0;
    int j1;
    int lastRow;
    string directory;
    map<int, FILE*> specialRows;
    int element_size;

	pthread_mutex_t mutex;

    void getFileName(int i, bool temporary, char* str);
};

#endif	/* _SPECIALROWWRITER_HPP */

