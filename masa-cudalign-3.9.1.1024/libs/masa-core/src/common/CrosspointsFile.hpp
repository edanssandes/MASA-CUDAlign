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

#ifndef _CROSSPOINTSFILE_HPP
#define	_CROSSPOINTSFILE_HPP

#include <stdio.h>
#include <vector>
#include <string>
using namespace std;

#include "Crosspoint.hpp"

class CrosspointsFile : public std::vector<crosspoint_t> {
    public:
        CrosspointsFile ( string filename );
        virtual ~CrosspointsFile();
        void loadCrosspoints();
        void reverse(int seq0_len, int seq1_len);

        void setAutoSave();
        void write ( int i, int j, int score, int type );
        void write(crosspoint_t crosspoint);
        void close();

        void writeToFile( string filename );

        void save();

        int getLargestPartitionSize(int* max_i=NULL, int* max_j=NULL);
    private:
        string filename;
        string tmpFilename;
        FILE* file;
        bool autoSave;

        void open();
};

#endif	/* _CROSSPOINTSFILE_HPP */

