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

#include "AbstractAlignerSafe.hpp"

#include <stdio.h>
#include <stdlib.h>

AbstractAlignerSafe::AbstractAlignerSafe() {
    pthread_mutex_init(&mutex, NULL);
    dispatcherQueueActive = false;

}

AbstractAlignerSafe::~AbstractAlignerSafe() {
	destroyDispatcherQueue();
}

void AbstractAlignerSafe::dispatchColumn(int j, const cell_t* buffer, int len) {
	pthread_mutex_lock(&mutex);
	AbstractAligner::dispatchColumn(j, buffer, len);
	pthread_mutex_unlock(&mutex);
}

void AbstractAlignerSafe::dispatchRow(int i, const cell_t* buffer, int len) {
	pthread_mutex_lock(&mutex);
	AbstractAligner::dispatchRow(i, buffer, len);
	pthread_mutex_unlock(&mutex);
}

void AbstractAlignerSafe::dispatchScore(score_t score, int bx, int by) {
	pthread_mutex_lock(&mutex);
	if (dispatcherQueueActive) {
		dispatch_job_t job;
		job.type = dispatch_job_t::JOB_SCORE;
		dispatch_job_t::dispatch_params_t::params_score_t params;
		params.score = score;
		params.bx = bx;
		params.by = by;
		job.dispatch_params.params_score = params;
		dispatcherQueue.push(job);
		pthread_cond_signal(&condition);
	} else {
		AbstractAligner::dispatchScore(score, bx, by);
	}
	printf ("%p: DispatchScore(%d,%d,%d): %d %d\n", pthread_self(), score.score, score.i, score.j, dispatcherQueueActive, dispatcherQueue.size());
	pthread_mutex_unlock(&mutex);
}



void AbstractAlignerSafe::createDispatcherQueue() {
	if (dispatcherQueueActive) {
		return;
	}
	dispatcherQueueActive = true;
    pthread_cond_init(&condition, NULL);
    int rc = pthread_create(&thread, NULL, staticFunctionThread, (void *)this);
    if (rc){
        fprintf(stderr, "setLogFile ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
    }
}

void AbstractAlignerSafe::destroyDispatcherQueue() {
	if (!dispatcherQueueActive) {
		return;
	}

	pthread_mutex_lock(&mutex);
	this->dispatcherQueueActive = false;
	pthread_cond_signal(&condition);
    pthread_mutex_unlock(&mutex);

    pthread_join(thread, NULL);
}

void *AbstractAlignerSafe::staticFunctionThread(void *arg) {
	AbstractAlignerSafe* obj = (AbstractAlignerSafe*)arg;
	obj->executeLoop();
    return NULL;
}

void AbstractAlignerSafe::executeLoop() {
	bool done = false;
	dispatch_job_t job;
	if (!dispatcherQueueActive) {
		done = true;
	}
	while (!done) {
	    pthread_mutex_lock(&mutex);
        while (dispatcherQueueActive && dispatcherQueue.size() == 0) {
        	printf ("%p: Before Cond Wait %d %d\n", pthread_self(), dispatcherQueueActive, dispatcherQueue.size());
        	pthread_cond_wait(&condition, &mutex);
        }
       	printf ("%p: After Cond Wait %d %d\n", pthread_self(), dispatcherQueueActive, dispatcherQueue.size());
        if (!dispatcherQueueActive && dispatcherQueue.size() == 0) {
        	done = true;
        } else {
        	job = dispatcherQueue.front();
           	printf ("%p: Before queue::pop() %d\n", pthread_self(), dispatcherQueue.size());
        	dispatcherQueue.pop();
           	printf ("%p: After queue::pop() %d\n", pthread_self(), dispatcherQueue.size());
        }
        //pthread_cond_signal(&condition);
	    pthread_mutex_unlock(&mutex);

	    if (!done) {
			if (job.type == dispatch_job_t::JOB_SCORE) {
				dispatch_job_t::dispatch_params_t::params_score_t params;
				params = job.dispatch_params.params_score;
				AbstractAligner::dispatchScore(params.score, params.bx, params.by);
				printf ("%p: AbstractAligner::dispatchScore() %d\n", pthread_self(), dispatcherQueue.size());
			}
	    }
	}
}

