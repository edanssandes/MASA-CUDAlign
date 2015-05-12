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

#include "RecurrentTimer.hpp"

#include <stdio.h>
#include <stdlib.h>

RecurrentTimer::RecurrentTimer(void (*routine)(float)) {
	this->routine = routine;
	this->active = false;

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&condition, NULL);
}

RecurrentTimer::~RecurrentTimer() {
	stop();
}

void RecurrentTimer::start(float interval) {
	if (this->active) {
		return;
	}

	this->interval = (int)(interval*1000);
	if (this->interval < 1) {
		this->interval = 1; // minimum interval (1ms).
	}
	this->active = true;
    int rc = pthread_create(&thread, NULL, staticFunctionThread, (void *)this);
    if (rc){
        fprintf(stderr, "setLogFile ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
    }
}

void RecurrentTimer::stop() {
	if (!this->active) {
		return;
	}
	pthread_mutex_lock(&mutex);
	this->active = false;
	pthread_cond_signal(&condition);
    pthread_mutex_unlock(&mutex);

    pthread_join(thread, NULL);
}


void RecurrentTimer::executeLoop() {
    timeval start;
    gettimeofday(&start, NULL);
    routine(0.0f);
	while (active) {
	    timeval event;
	    gettimeofday(&event, NULL);

		struct timespec time;
		time.tv_sec = event.tv_sec + (interval/1000);
		time.tv_nsec = event.tv_usec*1000 + (interval%1000)*1000000;
		if (time.tv_nsec >= 1000000000) {
			time.tv_nsec -= 1000000000;
			time.tv_sec++;
		}

	    pthread_mutex_lock(&mutex);
        pthread_cond_timedwait(&condition, &mutex, &time);
	    pthread_mutex_unlock(&mutex);

	    routine(getElapsedTime(&event, &start));
	}
}

void *RecurrentTimer::staticFunctionThread(void *arg) {
	RecurrentTimer* timer = (RecurrentTimer*)arg;
	timer->executeLoop();
    return NULL;
}

void RecurrentTimer::logNow() {
	pthread_cond_signal(&condition);
}

float RecurrentTimer::getElapsedTime(timeval *end_time, timeval *start_time) {
	timeval temp_diff;

	temp_diff.tv_sec = end_time->tv_sec - start_time->tv_sec;
	temp_diff.tv_usec = end_time->tv_usec - start_time->tv_usec;

	if (temp_diff.tv_usec < 0) {
		long long nsec = temp_diff.tv_usec/1000000;
		temp_diff.tv_usec += 1000000 * nsec;
		temp_diff.tv_sec -= nsec;
	}

	return (1000000LL * temp_diff.tv_sec + temp_diff.tv_usec)/1000000.0f;
}
