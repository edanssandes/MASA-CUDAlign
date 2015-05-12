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

#include "BestScoreList.hpp"

#include <stdlib.h>
#include <stdio.h>

#define DEBUG (0)


/*#define MY_MUTEX_INIT
#define MY_MUTEX_DESTROY
#define MY_MUTEX_LOCK
#define MY_MUTEX_UNLOCK
*/

#define MY_MUTEX_INIT		pthread_mutex_init(&mutex, NULL);
#define MY_MUTEX_DESTROY	pthread_mutex_destroy(&mutex);
#define MY_MUTEX_LOCK		pthread_mutex_lock(&mutex);
#define MY_MUTEX_UNLOCK		pthread_mutex_unlock(&mutex);



BestScoreList::BestScoreList(int limit, int min_score, int seq0_len, int seq1_len, const score_params_t* score_params)
{
	this->limit = limit;
	this->min_score = min_score;
	this->seq0_len = seq0_len;
	this->seq1_len = seq1_len;
	this->score_params = score_params;

	MY_MUTEX_INIT
}

BestScoreList::~BestScoreList() {
	MY_MUTEX_DESTROY
}


score_t BestScoreList::getBestScore() const {
	set<score_t>::iterator it=begin();
	if (it == end()) {
		score_t null_score;
		null_score.score = -INF;
		null_score.i = -1;
		null_score.j = -1;
		return null_score;
	} else {
		return *it;
	}
}

bool BestScoreList::isDerived(const score_t best, const score_t score) {
	int diff_z = score.score - best.score;
	int diff_x = score.j - best.j;
	int diff_y = score.i - best.i;

	if (diff_z >= 0) {
		return false;
	}

	if (min_score >= 0 && best.score > (abs(diff_x) + abs(diff_y))/score_params->gap_ext) {
		return true;
	}
	if (min_score < 0) {
		int min_len = seq0_len < seq1_len ? seq0_len : seq1_len;
		if ((abs(diff_x) + abs(diff_y)) > min_len) {
			return true;
		}
	}

	int gaps;
	int diagonal;
	if ((diff_x <= 0 && diff_y <= 0) || (diff_x >= 0 && diff_y >= 0)) {
		gaps = abs(abs(diff_x)-abs(diff_y));
		diagonal = (abs(diff_x) < abs(diff_y)) ? abs(diff_x) : abs(diff_y);
	} else {
		gaps = abs(diff_x)+abs(diff_y);
		diagonal = (abs(diff_x) < abs(diff_y)) ? abs(diff_x) : abs(diff_y);
	}
	const float prob_match = 0.25f;
	const float prob_gap_adj = 2.0f;
	/*if (best.z > 0) {
		gap_extra_adj = best.z/DNA_GAP_EXT;
	}*/

	int z_min = (score_params->mismatch*(1-prob_match) + score_params->match*prob_match)*diagonal;
	int z_max = score_params->match*diagonal;
	int g_diff = score_params->gap_ext*gaps;

	return (diff_z >= z_min - g_diff*prob_gap_adj) && (diff_z <= z_max - g_diff/prob_gap_adj);
}

bool BestScoreList::isAllowed(const score_t best, const score_t score) {
	if (best.score > 0 && score.score < 0.25*best.score) {
		return false;
	}
	if (best.score < 0 && score.score < 1.10*best.score) {
		return false;
	}
	return true;
}

void BestScoreList::add(int i, int j, int score) {
	MY_MUTEX_LOCK
	_add(i,j,score);
	MY_MUTEX_UNLOCK
}

void BestScoreList::_add(int i, int j, int score) {
	if (score < min_score) return;
	score_t reg;
	reg.score = score;
	reg.i = i;
	reg.j = j;
	//int3 reg = make_int3(j, i, score);

	if (size() == limit) {
		set<score_t>::iterator it=end();
		it--;
		if (it->score > score) {
			return;
		}
	}

	if (size() > 0) {
		set<score_t>::iterator best=begin();
		if (!isAllowed(*best, reg)) return;
	}

	if (find(reg) != end()) {
		return;
	}

	bool derived = false;
	for (set<score_t>::iterator it=begin() ; it != end(); it++ ) {
		if (isDerived(*it, reg)) {
			derived = true;
			//printf("HEY NOT: (%d,%d,%d) -> (%d,%d,%d)\n", reg.y, reg.x, reg.z, it->y, it->x, it->z);
			break;
		}
	}

	if (!derived) {
		set<score_t>::iterator it=begin();
		while (it != end()) {
			if (isDerived(reg, *it) || !isAllowed(reg, *it)) {
				set<score_t>::iterator toErase = it;
				it++;
				erase(toErase);
				//printf("HEY ERASE: (%d,%d,%d) -> (%d,%d,%d)\n", reg.y, reg.x, reg.z, it->y, it->x, it->z);
			} else {
				it++;
			}
		}


		insert(reg);
		if (size() > limit) {
			set<score_t>::iterator it=end();
			it--;
			erase(it);
		}
		if (DEBUG) {
			int c = 1;
			printf(" I: %d,%d,%d\n", reg.i, reg.j, reg.score);
			for (set<score_t>::iterator it=begin() ; it != end(); it++ ) {
				printf("%2d: %d,%d,%d\n", c++, it->i, it->j, it->score);
			}
			printf("----\n");
		}
	}



}

/*
g++ -pthread src/common/BestScoreList.cpp -o test && ./test
void* testFunctionThread(void* args) {
	BestScoreList* list = (BestScoreList*)args;
	for (int k=0; k<100000; k++) {
		int i = (int)((float)rand()/RAND_MAX*1000000);
		int j = (int)(((float)rand()/RAND_MAX*1000)*((float)rand()/RAND_MAX*1000));
		int s = (int)(((float)rand()/RAND_MAX*102)*((float)rand()/RAND_MAX*50)*((float)rand()/RAND_MAX*70));
		if (k%1000==0) printf("%d: add[%d,%d,%d]\n", k, i, j, s);
		list->add(i, j, s);
	}
}

int main() {
	pthread_t thread[400];
	score_params_t params;
	params.match = 1;
	params.mismatch = -3;
	params.gap_open = 3;
	params.gap_ext = 2;
	BestScoreList* writer = new BestScoreList(1000, 0, 1000000, 1000000, &params);
	for (int i=0; i<50; i++) {
		//sleep(1);
		int rc = pthread_create(&thread[i], NULL, testFunctionThread, (void *)writer);
		printf("pthread_create(thread[%d]): %d\n", i, rc);
	}
	for (int i=0; i<50; i++) {
		pthread_join(thread[i], NULL);
	}
}
*/
