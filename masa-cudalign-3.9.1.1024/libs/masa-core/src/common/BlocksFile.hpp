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

#ifndef BLOCKSFILE_HPP_
#define BLOCKSFILE_HPP_

#include <string>
using namespace std;

#include "../libmasa/Grid.hpp"

class BlocksFile {
public:
	BlocksFile(string filename);
	virtual ~BlocksFile();

	void initialize(const Grid* grid);
	void setScore(int bx, int by, int score);
	void close();
	bool isInitialized();

	int* reduceData(int &bh, int &bw);
private:

	bool initialized;
	int width;
	int height;
	FILE* file;
	string filename;
	int* buffer;
};

#endif /* BLOCKSFILE_HPP_ */
