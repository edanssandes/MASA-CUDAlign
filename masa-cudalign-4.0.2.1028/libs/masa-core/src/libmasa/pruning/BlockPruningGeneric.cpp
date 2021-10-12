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

#include "BlockPruningGeneric.hpp"

#include <stdio.h>
#include "../libmasa.hpp"



BlockPruningGeneric::BlockPruningGeneric() {
	this->scoreHorizontal = NULL;
	this->scoreVertical = NULL;
}

BlockPruningGeneric::~BlockPruningGeneric() {
	finalize();
}

void BlockPruningGeneric::initialize() {
	allocateScoreHorizontal();
	allocateScoreVertical();
}

void BlockPruningGeneric::finalize() {
	if (this->scoreHorizontal != NULL) {
		delete[] this->scoreHorizontal;
		this->scoreHorizontal = NULL;
	}
	if (this->scoreVertical != NULL) {
		delete[] this->scoreVertical;
		this->scoreVertical = NULL;
	}
}

void BlockPruningGeneric::pruningUpdate(int bx, int by, int score) {
	updateBestScore(score);

	if (isBlockPrunable(bx, by, score)) {
		scoreHorizontal[bx] = by+1;
		scoreVertical[by] = bx+1;
		//printf("Prunable: (%d,%d)\n", bx, by);
	}
	//printf("Update: (%d,%d). H: %d    Hmax: %d    Best: %d\n", bx, by, score, max, bestScore);

}

bool BlockPruningGeneric::isBlockPruned(int bx, int by) {
	if (bx == 0 && by == 0) {
		return false;
	} else if (scoreHorizontal[bx] == by && scoreVertical[by] == bx) {
		scoreHorizontal[bx] = by+1;
		scoreVertical[by] = bx+1;
		//printf("Pruned: (%d,%d)\n", bx, by);
		return true;
	} else {
		return false;
	}
}

void BlockPruningGeneric::allocateScoreVertical() {
	if (this->scoreVertical != NULL) {
		delete[] this->scoreVertical;
	}
	this->scoreVertical = new int[getGrid()->getGridHeight()];
	memset(this->scoreVertical, 0, sizeof(int)*getGrid()->getGridHeight());
}

void BlockPruningGeneric::allocateScoreHorizontal() {
	if (this->scoreHorizontal != NULL) {
		delete[] this->scoreHorizontal;
	}
	this->scoreHorizontal = new int[getGrid()->getGridWidth()];
	memset(this->scoreHorizontal, 0, sizeof(int)*getGrid()->getGridWidth());
}

