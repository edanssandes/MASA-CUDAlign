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

#ifndef BESTSCORELIST_H_
#define BESTSCORELIST_H_
#include <pthread.h>
#include <set>
using namespace std;

#include "../libmasa/libmasa.hpp"

struct classcomp {
	bool operator() (const score_t& a, const score_t& b) const {
		int diff_z = a.score - b.score;
		if (diff_z) return diff_z > 0;
		int diff_y = b.i - a.i;
		if (diff_y) return diff_y > 0;
		return b.j > a.j;
	}
};


class BestScoreList : public std::set<score_t,classcomp> {
public:
	BestScoreList(int limit, int min_score, int seq0_len, int seq1_len, const score_params_t* score_params);
	virtual ~BestScoreList();
	void add(int i, int j, int score);
	score_t getBestScore() const;
private:
	int limit;
	int min_score;
	int seq0_len;
	int seq1_len;
	const score_params_t* score_params;

	//set<int3,classcomp> bestScores;
	pthread_mutex_t mutex;

	bool isDerived(const score_t best, const score_t score);
	bool isAllowed(const score_t best, const score_t score);
	void _add(int i, int j, int score);
};

#endif /* BESTSCORELIST_H_ */
