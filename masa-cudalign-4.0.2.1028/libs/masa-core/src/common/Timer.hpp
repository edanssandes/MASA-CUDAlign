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

#ifndef _TIMER_HPP
#define	_TIMER_HPP

#include <vector>
#include <map>
#include <string>
#include <stdio.h>
using namespace std;
#include <sys/time.h>


class Timer {
public:
    Timer();
    virtual ~Timer();

    void init();
    int createEvent(string name);
	float eventRecord(int event);
    float printStatistics(FILE* file=stdout);
	float totalTime();
    bool intervalElapsed ( float interval );
    static float getGlobalTime();
private:
    struct stat_t {
        string name;
        float sum;
        int count;
        struct timeval event;
    };

	bool hasEvent;
	timeval previousEvent;
	timeval repeatedEvent;
	timeval intervalEvent;
	timeval startEvent;
	static timeval staticEvent;
	static bool hasStaticEvent;
	
    vector<stat_t> stats;

    static float getElapsedTime(timeval *end_time, timeval *start_time);
};

#endif	/* _TIMER_HPP */

