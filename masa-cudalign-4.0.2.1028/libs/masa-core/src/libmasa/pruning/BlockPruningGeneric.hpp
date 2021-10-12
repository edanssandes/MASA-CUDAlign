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

class BlockPruningGeneric;

#ifndef BLOCKPRUNINGGENERIC_HPP_
#define BLOCKPRUNINGGENERIC_HPP_

#include "../pruning/AbstractBlockPruning.hpp"

class BlockPruningGeneric : public AbstractBlockPruning {
public:
	BlockPruningGeneric();
	virtual ~BlockPruningGeneric();


	virtual void pruningUpdate(int bx, int by, int score);
	virtual bool isBlockPruned(int bx, int by);

private:
	int* scoreVertical;
	int* scoreHorizontal;

	void allocateScoreVertical();
	void allocateScoreHorizontal();

	virtual void initialize();
	virtual void finalize();
};

#endif /* BLOCKPRUNINGGENERIC_HPP_ */
