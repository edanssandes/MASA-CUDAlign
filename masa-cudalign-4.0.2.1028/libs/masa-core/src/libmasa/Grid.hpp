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

class Grid;

#ifndef GRID_HPP_
#define GRID_HPP_

#include <stdio.h>

#include "Partition.hpp"

class Grid {
public:
	Grid(Partition partition);
	virtual ~Grid();

	void setBlockHeight(int blockHeight);
	void setBlockWidth(int blockWidth);

	void splitGridVertically(int count);
	void splitGridHorizontally(int count);

	void splitGridVertically(int count, int* splits);
	void splitGridHorizontally(int count, int* splits);

	void getBlockPosition(int bx, int by, int* i0, int* j0, int* i1=NULL, int* j1=NULL) const;

	void setMinBlockSize(int minBlockWidth, int minBlockHeight);

	void setAdjustment(int adjustment);
	int  getBlockAdjustment(int bx, int by) const;

	int getGridWidth() const;
	int getGridHeight() const;

	int getBlockWidth(int bx, int by) const;
	int getBlockHeight(int bx, int by) const;

	int getHeight() const;
	int getWidth() const;

	const int* getBlockSplitHorizontal() const;
	const int* getBlockSplitVertical() const;

private:
	int  blockWidth;
	int  blockHeight;
	int  adjustment;
	int  minBlockWidth;
	int  minBlockHeight;

	/*
	 * The split vectors will contains the start coordinates of each block.
	 * The first element will always be 0 and the last element will have the
	 * dimension size. Note that the vector must contain N+1 elements, so the
	 * last element is blockSplitXXX[blockCountXXX].
	 */
	int  blockCountHorizontal;
	int* blockSplitHorizontal;
	int  blockCountVertical;
	int* blockSplitVertical;

	int width;
	int height;
	Partition partition;

	void getBlockPositionH(int bx, int by, int* j0, int* j1) const;
	void getBlockPositionV(int bx, int by, int* i0, int* i1) const;

};

#endif /* GRID_HPP_ */
