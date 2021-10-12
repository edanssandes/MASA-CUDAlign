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

#ifndef BLOCKPRUNINGGENERICN2_HPP_
#define BLOCKPRUNINGGENERICN2_HPP_

#include "AbstractBlockPruning.hpp"

class BlockPruningGenericN2: public AbstractBlockPruning {
public:
	BlockPruningGenericN2();
	virtual ~BlockPruningGenericN2();

	virtual void pruningUpdate(int bx, int by, int score);
	virtual bool isBlockPruned(int bx, int by);
private:
	int** k;
	int gridHeight;
	int gridWidth;

	virtual void initialize();
	virtual void finalize();
};

#endif /* BLOCKPRUNINGGENERICN2_HPP_ */
