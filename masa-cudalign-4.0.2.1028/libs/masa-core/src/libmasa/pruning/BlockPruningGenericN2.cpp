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

#include "BlockPruningGenericN2.hpp"

#include <stdio.h>
#include <string.h>

BlockPruningGenericN2::BlockPruningGenericN2() {
	this->k = NULL;
	this->gridHeight = 0;
	this->gridWidth = 0;
}

BlockPruningGenericN2::~BlockPruningGenericN2() {
	finalize();
}

void BlockPruningGenericN2::pruningUpdate(int bx, int by, int score) {
	if (getGrid() == NULL) return;
	updateBestScore(score);

	if (isBlockPrunable(bx, by, score)) {
		k[by+1][bx+1] = true;
	}
}

bool BlockPruningGenericN2::isBlockPruned(int bx, int by) {
	if (getGrid() == NULL) return false;
	if (k[by+1][bx] && k[by][bx] && k[by][bx+1]) {
		k[by+1][bx+1] = true;
		return true;
	} else {
		return false;
	}
}

void BlockPruningGenericN2::initialize() {
	gridHeight = getGrid()->getGridHeight();
	gridWidth = getGrid()->getGridWidth();

	this->k = new int*[gridHeight+1];
	for (int i=0; i<=gridHeight; i++) {
		this->k[i] = new int[gridWidth+1];
		memset(this->k[i], 0, sizeof(int)*(gridWidth+1));
	}
	for (int i=1; i<=gridHeight; i++) {
		this->k[i][0] = true;
	}
	for (int i=1; i<=gridWidth; i++) {
		this->k[0][i] = true;
	}
}

void BlockPruningGenericN2::finalize() {
	if (this->k != NULL) {

		for (int i=0; i<=gridHeight; i++) {
			delete[] this->k[i];
		}
		delete[] this->k;

		this->k = NULL;
		this->gridHeight = 0;
		this->gridWidth = 0;
	}
}
