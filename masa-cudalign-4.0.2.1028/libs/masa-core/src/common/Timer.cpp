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

#include "Timer.hpp"

#include <stdio.h>

bool Timer::hasStaticEvent = false;
timeval Timer::staticEvent;

float Timer::getGlobalTime() {
	if (!hasStaticEvent) {
		hasStaticEvent = true;
	    gettimeofday(&staticEvent, NULL);
	}

    timeval event;
    gettimeofday(&event, NULL);

    float diff = getElapsedTime(&event, &staticEvent);
    return diff;
}


float Timer::getElapsedTime(timeval *end_time, timeval *start_time) {
	timeval temp_diff;

	temp_diff.tv_sec = end_time->tv_sec - start_time->tv_sec;
	temp_diff.tv_usec = end_time->tv_usec - start_time->tv_usec;

	if (temp_diff.tv_usec < 0) {
		long long nsec = temp_diff.tv_usec/1000000;
		temp_diff.tv_usec += 1000000 * nsec;
		temp_diff.tv_sec -= nsec;
	}

	return (1000000LL * temp_diff.tv_sec + temp_diff.tv_usec)/1000.0f;

}

Timer::Timer() {
    /*unsigned int timer = 0;
    cutilCheckError( cutCreateTimer( &timer));
    cutilCheckError( cutStartTimer( timer));*/
	hasEvent = 0;
	//previousEvent = NULL;

	gettimeofday(&startEvent, NULL);
	gettimeofday(&intervalEvent, NULL);
	gettimeofday(&previousEvent, NULL);

	//init();
}

Timer::~Timer() {
}

int Timer::createEvent(string name) {
    stat_t stat;
    stat.name = name;
    stat.sum = 0;
    stat.count = 0;
    int id = stats.size();
    stats.push_back(stat);
	
    return id;

}

void Timer::init() {
	gettimeofday(&startEvent, NULL);
	gettimeofday(&intervalEvent, NULL);
	gettimeofday(&previousEvent, NULL);
	hasEvent = true;
}

float Timer::eventRecord(int id) {
    float diff = 0;
    timeval currentEvent = stats[id].event;

    gettimeofday(&currentEvent, NULL);

    if (hasEvent) {
		diff = getElapsedTime(&currentEvent, &previousEvent);
        stats[id].sum += diff;
        stats[id].count++;
    }
    previousEvent = currentEvent;
	hasEvent = true;
    return diff;
}

float Timer::totalTime() {
    timeval currentEvent;
    gettimeofday(&currentEvent, NULL);
	float diff = getElapsedTime(&currentEvent, &startEvent);
	return diff;
}

bool Timer::intervalElapsed ( float interval ) {
    timeval currentEvent;
    gettimeofday(&currentEvent, NULL);
	float diff = getElapsedTime(&currentEvent, &intervalEvent);
	if (diff/1000 >= interval) {
		//printf("OK||||||\n");
		intervalEvent = currentEvent;
		return true;
	} else {
		//printf("%f - %f\n", diff/1000, interval);
		return false;
	}
}


float Timer::printStatistics(FILE* file) {
    float sum = 0;
    for (int i = 0; i<stats.size(); i++) {
        stat_t stat = stats[i];
		fprintf(file, "%15s: %12.4f (%4d)  avg.: %8.4f\n", stat.name.c_str(), stat.sum, stat.count, stat.sum/stat.count);
        sum += stat.sum;
    }
	fprintf(file, "%15s: %12.4f\n", "TOTAL", sum);
	fflush(file);
    return sum;
}


