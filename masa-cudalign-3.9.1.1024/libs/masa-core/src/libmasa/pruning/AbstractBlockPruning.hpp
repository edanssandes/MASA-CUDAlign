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

#ifndef ABSTRACTBLOCKPRUNING_HPP_
#define ABSTRACTBLOCKPRUNING_HPP_

#include "../libmasaTypes.hpp"
#include "../Grid.hpp"
#include "../IManager.hpp" // for the recurrence types

class AbstractBlockPruning {
public:
	AbstractBlockPruning();
	virtual ~AbstractBlockPruning();

	void updateBestScore(int score);
	const Grid* getGrid() const;
	void setGrid(const Grid* grid);
	void setSuperPartition(Partition superPartition);
	void setScoreParams(const score_params_t* score_params);
	void setLocalAlignment();
	void setGlobalAlignment();
	int getRecurrenceType() const;
	void setRecurrenceType(int recurrenceType);

protected:
	bool isBlockPrunable(int bx, int by, int score);



private:
	const score_params_t* score_params;

	int  max_i;
	int  max_j;

	int  bestScore;

	int  recurrenceType;

	const Grid* grid;


	virtual void initialize() = 0;
	virtual void finalize() = 0;
};

#endif /* ABSTRACTBLOCKPRUNING_HPP_ */
