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

class BlockPruningDiagonal;

#ifndef BLOCKPRUNERDIAGONAL_HPP_
#define BLOCKPRUNERDIAGONAL_HPP_

#include "AbstractBlockPruning.hpp"

class BlockPruningDiagonal : public AbstractBlockPruning {
public:
	virtual ~BlockPruningDiagonal();
	BlockPruningDiagonal();

	virtual void initialize();
	virtual void finalize();

	void updatePruningWindow(int diagonal, const score_t* block_scores);
	void getNonPrunableWindow(int* start, int* end);

//	void setBlockHeight(int blockHeight);
//	void setBlockWidth(int blockWidth);
//
//	void setBlockSplitHorizontal(int count, const int* splits);
//	void setBlockSplitVertical(int count, const int* splits);
//
//	void initializePrunableWindows();

private:
//	const score_params_t* score_params;

//	int  seq0_len;
//	int  seq1_len;
//
//	int  bestScore;
//
//	int  blockWidth;
//	int  blockHeight;
//	int  blockCountHorizontal;
//	const int* blockSplitHorizontal;
//	int  blockCountVertical;
//	const int* blockSplitVertical;

	int windowStart;
	int windowEnd;

	//int updateBestScore(const score_t* block_scores);
	//int isBlockPrunable(const int bx, const int by, score_t block_score, int best_score);
	//void getBlockPosition(int bx, int by, int* row, int* column);
};

#endif /* BLOCKPRUNERDIAGONAL_HPP_ */
