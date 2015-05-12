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

#include "Grid.hpp"

#include <stdio.h>
#include "utils/AlignerUtils.hpp"

Grid::Grid(Partition partition) {
	this->partition = partition;
	this->blockSplitHorizontal = NULL;
	this->blockSplitVertical = NULL;

	this->blockCountHorizontal = 0;
	this->blockCountVertical = 0;

	this->blockHeight = 0;
	this->blockWidth = 0;

	this->width = partition.getWidth();
	this->height = partition.getHeight();

	this->adjustment = 0;

	setMinBlockSize(1, 1);
}

Grid::~Grid() {
	delete blockSplitHorizontal;
	delete blockSplitVertical;
}


void Grid::setBlockHeight(int blockHeight) {
	if (blockHeight <= minBlockHeight) {
		blockHeight = minBlockHeight;
	}
	this->blockHeight = blockHeight;
	this->blockCountVertical = (height+blockHeight-1)/blockHeight;
	this->blockSplitVertical = NULL;
}

void Grid::setBlockWidth(int blockWidth) {
	if (blockWidth <= minBlockWidth) {
		blockWidth = minBlockWidth;
	}
	this->blockWidth = blockWidth;
	this->blockCountHorizontal = (width+blockWidth-1)/blockWidth;
	this->blockSplitHorizontal = NULL;
}


void Grid::splitGridVertically(int count, int* splits) {
	if (blockSplitHorizontal != NULL) {
		delete blockSplitHorizontal;
	}
	this->blockHeight = -1;
	this->blockCountVertical = count;
	this->blockSplitVertical = splits;

}

void Grid::splitGridHorizontally(int count, int* splits) {
	if (blockSplitVertical != NULL) {
		delete blockSplitVertical;
	}
	this->blockWidth = -1;
	this->blockCountHorizontal = count;
	this->blockSplitHorizontal = splits;
}

/**
 * Splits the grid in $count$ positions (vertically). If $count$ is greater
 * than the sequence size, then each partition has the almost the same height,
 * with a maximum difference of $1$.
 *
 * @param count number of parts to split the grid.
 */
void Grid::splitGridVertically(int count) {
	if (blockSplitVertical != NULL) {
		delete blockSplitVertical;
	}
	if (count > height/minBlockHeight) {
		count = height/minBlockHeight;
	}
	if (count < 1) {
		count = 1;
	}
	this->blockHeight = -1;
	this->blockCountVertical = count;
	this->blockSplitVertical = new int[count+1];

	// Splits the grid into blocks with almost equal size.
	AlignerUtils::splitBlocksEvenly(blockSplitVertical, partition.getI0(), partition.getI1(), count);
}

/**
 * Splits the grid in $count$ positions (horizontal). If $count$ is greater
 * than the sequence size, then each partition has the almost the same width,
 * with a maximum difference of $1$.
 *
 * @param count number of parts to split the grid.
 */
void Grid::splitGridHorizontally(int count) {
	if (blockSplitHorizontal != NULL) {
		delete blockSplitHorizontal;
	}
	if (count > width/minBlockWidth) {
		count = width/minBlockWidth;
	}
	if (count < 1) {
		count = 1;
	}
	this->blockWidth = -1;
	this->blockCountHorizontal = count;
	this->blockSplitHorizontal = new int[count+1];

	// Splits the grid into blocks with almost equal size.
	AlignerUtils::splitBlocksEvenly(blockSplitHorizontal, partition.getJ0(), partition.getJ1(), count);
}


void Grid::getBlockPositionV(int bx, int by, int* i0, int* i1) const {
	if (bx < 0 || by < 0 || bx >= blockCountHorizontal || by >= blockCountVertical) {
		if (i0 != NULL) *i0 = -1;
		if (i1 != NULL) *i1 = -1;
	} else {
		if (blockSplitVertical != NULL) {
			if (i0 != NULL) *i0 = blockSplitVertical[by];
			if (i1 != NULL) *i1 = blockSplitVertical[by+1];
		} else {
			if (i0 != NULL) *i0 = partition.getI0() + by*blockHeight;
			if (i1 != NULL) {
				*i1 = partition.getI0() + (by+1)*blockHeight;
				if (*i1 > partition.getI1()) {
					*i1 = partition.getI1();
				}
			}
		}
	}
}

void Grid::getBlockPositionH(int bx, int by, int* j0, int* j1) const {
	if (bx < 0 || by < 0 || bx >= blockCountHorizontal || by >= blockCountVertical) {
		if (j0 != NULL) *j0 = -1;
		if (j1 != NULL) *j1 = -1;
	} else {
		if (blockSplitHorizontal != NULL) {
			if (j0 != NULL) *j0 = blockSplitHorizontal[bx];
			if (j1 != NULL) *j1 = blockSplitHorizontal[bx+1];
		} else {
			if (j0 != NULL) *j0 = partition.getJ0() + bx*blockWidth;
			if (j1 != NULL) {
				*j1 = partition.getJ0() + (bx+1)*blockWidth;
				if (*j1 > partition.getJ1()) {
					*j1 = partition.getJ1();
				}
			}
		}
	}
}

void Grid::getBlockPosition(int bx, int by, int* i0, int* j0, int* i1, int* j1) const {
	getBlockPositionH(bx, by, j0, j1);
	getBlockPositionV(bx, by, i0, i1);
}

const int* Grid::getBlockSplitHorizontal() const {
	return blockSplitHorizontal;
}

const int* Grid::getBlockSplitVertical() const {
	return blockSplitVertical;
}

void Grid::setMinBlockSize(int minBlockWidth, int minBlockHeight) {
	this->minBlockWidth = minBlockWidth;
	this->minBlockHeight = minBlockHeight;
}

void Grid::setAdjustment(int adjustment) {
	this->adjustment = adjustment;
}

int Grid::getBlockAdjustment(int bx, int by) const {
	return adjustment;
}

int Grid::getGridWidth() const {
	return blockCountHorizontal;
}

int Grid::getGridHeight() const {
	return blockCountVertical;
}

int Grid::getBlockWidth(int bx, int by) const {
	int j0;
	int j1;
	getBlockPositionH(bx, by, &j0, &j1);
	return j1-j0;
}

int Grid::getBlockHeight(int bx, int by) const {
	int i0;
	int i1;
	getBlockPositionV(bx, by, &i0, &i1);
	return i1-i0;
}

int Grid::getHeight() const {
	return height;
}

int Grid::getWidth() const {
	return width;
}
