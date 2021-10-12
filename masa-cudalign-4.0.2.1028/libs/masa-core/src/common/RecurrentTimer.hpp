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

#ifndef RECURRENTTIMER_HPP_
#define RECURRENTTIMER_HPP_

#include <pthread.h>
#include <sys/time.h>

class RecurrentTimer {
public:
	RecurrentTimer(void (*routine)(float));
	virtual ~RecurrentTimer();
	void start(float interval);
	void stop();
	void logNow();

private:
	static void *staticFunctionThread(void *arg);
	void executeLoop();

	void (*routine)(float);
	int interval;
	bool active;

	pthread_t thread;
	pthread_mutex_t mutex;
	pthread_cond_t condition;

	float getElapsedTime(timeval *end_time, timeval *start_time);
};

#endif /* RECURRENTTIMER_HPP_ */
