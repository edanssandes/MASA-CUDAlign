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

#include "Buffer2.hpp"

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
//#include <sys/time.h>

#include "../Timer.hpp"
#include <time.h>
#include <unistd.h>
#include "BufferLogger.hpp"

#define DEBUG (0)

Buffer2::Buffer2(int buffer_max) {
    this->buffer_max = buffer_max;
    buffer_size = buffer_max+5;
    buffer_start = 0;
    buffer_end = 0;
    buffer = (cell_t*)malloc(buffer_size*sizeof(cell_t));
    destroyed = false;
	isLogging = false;
    
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&notFullCond, NULL);
    pthread_cond_init(&notEmptyCond, NULL);
    pthread_cond_init(&emptyCond, NULL);
    pthread_cond_init(&loggerCond, NULL);

	stats.bufferUsage = 0;
	stats.totalReadBytes = 0;
	stats.blockingReadTime = 0;
	stats.totalWriteBytes = 0;
	stats.blockingWriteTime = 0;

	tempBlockingReadTime = -1;
	tempBlockingWriteTime = -1;

    /*mutex = PTHREAD_MUTEX_INITIALIZER;
    notFullCond = PTHREAD_COND_INITIALIZER;
    notEmptyCond = PTHREAD_COND_INITIALIZER;
    emptyCond = PTHREAD_COND_INITIALIZER;*/
}

Buffer2::~Buffer2() {
    fprintf(stderr, "Destruct Buffer...\n");
    free(buffer);
}

void Buffer2::destroy() {
	fprintf(stderr, "Buffer2::destroy()...\n");
    pthread_mutex_lock(&mutex);

    destroyed = true;
    pthread_cond_signal(&notFullCond);
    pthread_cond_signal(&notEmptyCond);
    pthread_cond_signal(&emptyCond);
    pthread_cond_signal(&loggerCond);

    pthread_mutex_unlock(&mutex);

    if (isLogging) {
    	isLogging = false;
    	pthread_join(loggerThread, NULL);
    }
	fprintf(stderr, "Buffer2::destroy() DONE\n");
}

bool Buffer2::isDestroyed() {
    return destroyed;
}

void Buffer2::waitEmptyBuffer() {
    pthread_mutex_lock(&mutex);
	while (sizeUsed() > 0 && !destroyed) {
		fprintf (stderr, "Waiting empty buffer: %d\n", sizeUsed());
        pthread_cond_wait (&emptyCond, &mutex);		
	}
    pthread_mutex_unlock(&mutex);
}

int Buffer2::circularLoad(cell_t* dst, int len) {
    
    if (buffer_start+len < buffer_size) {
        memcpy(dst, buffer+buffer_start, len*sizeof(cell_t));
        buffer_start += len;
    } else {
    	memcpy(dst, buffer+buffer_start, (buffer_size-buffer_start)*sizeof(cell_t));
    	memcpy(dst+(buffer_size-buffer_start), buffer, (len-(buffer_size-buffer_start))*sizeof(cell_t));
        buffer_start = len-(buffer_size-buffer_start);
    }
    if (sizeUsed() > 0) {
        pthread_cond_signal(&notFullCond);
    } else {
        pthread_cond_signal(&emptyCond);
	}
    return len;
}

int Buffer2::circularSkip(int len) {

    if (buffer_start+len < buffer_size) {
        buffer_start += len;
    } else {
        buffer_start = len-(buffer_size-buffer_start);
    }
    if (sizeUsed() > 0) {
        pthread_cond_signal(&notFullCond);
    } else {
        pthread_cond_signal(&emptyCond);
	}
    return len;
}


int Buffer2::circularStore(const cell_t* src, int len) {
    if (buffer_end+len < buffer_size) {
        memcpy(buffer+buffer_end, src, len*sizeof(cell_t));
        buffer_end += len;
    } else {
        memcpy(buffer+buffer_end, src, (buffer_size-buffer_end)*sizeof(cell_t));
        memcpy(buffer, src+(buffer_size-buffer_end), (len - (buffer_size-buffer_end))*sizeof(cell_t));
        buffer_end = len - (buffer_size-buffer_end);
    }
    if (sizeUsed() > 0) {
        pthread_cond_signal(&notEmptyCond);
    }
    return len;
}

int Buffer2::sizeAvailable() {
    return buffer_max - sizeUsed();
}

int Buffer2::sizeUsed() {
    if (buffer_end >= buffer_start) {
        return buffer_end - buffer_start;
    } else {
        return buffer_end - buffer_start + buffer_size;
    }
}

int Buffer2::readBuffer(cell_t* data, int nmemb)
{
	pthread_mutex_lock(&mutex);
	if (DEBUG) printf("Buffer2::readBuffer(%d) - buf: %d\n", nmemb, sizeUsed());
    int size_total = nmemb;
    int size_left = size_total;
    while ((sizeUsed() < size_left) && !destroyed) {
    	if (data == NULL) {
    		size_left -= circularSkip(sizeUsed());
    	} else {
    		size_left -= circularLoad(data+(size_total-size_left), sizeUsed());
    	}
        float t0 = Timer::getGlobalTime();
        tempBlockingReadTime = t0;
        pthread_cond_wait (&notEmptyCond, &mutex);
        tempBlockingReadTime = -1;
        float t1 = Timer::getGlobalTime();
        stats.blockingReadTime += (t1-t0);
    }
    if (!destroyed) {
    	if (data == NULL) {
    		size_left -= circularSkip(size_left);
    	} else {
    		size_left -= circularLoad(data+(size_total-size_left), size_left);
    	}
    }
    stats.bufferUsage = sizeUsed();
    if (stats.totalReadBytes == 0/* && inputBuffer*/) {
    	pthread_cond_signal(&loggerCond);
    }
    stats.totalReadBytes += (size_total-size_left);
    pthread_mutex_unlock(&mutex);
    
    if (size_left != 0) fprintf(stderr, "readBuffer: diff len: %d %d\n", size_total-size_left, size_total);
    
    return size_total-size_left;
}

int Buffer2::writeBuffer(const cell_t* data, int nmemb)
{
    pthread_mutex_lock(&mutex);
	if (DEBUG) printf("Buffer2::writeBuffer(%d) - buf: %d\n", nmemb, sizeUsed());
    int size_total = nmemb;
    int size_left = size_total;
    while ((sizeAvailable() < size_left) && !destroyed) {
        size_left -= circularStore(data+(size_total-size_left), sizeAvailable());
        float t0 = Timer::getGlobalTime();
        tempBlockingWriteTime = t0;
        pthread_cond_wait (&notFullCond, &mutex);
        tempBlockingWriteTime = -1;
        float t1 = Timer::getGlobalTime();
        stats.blockingWriteTime += (t1-t0);
    }
    if (!destroyed) {
        size_left -= circularStore(data+(size_total-size_left), size_left);
    }
    stats.bufferUsage = sizeUsed();
    if (stats.totalWriteBytes == 0/* && !inputBuffer*/) {
    	pthread_cond_signal(&loggerCond);
    }
    stats.totalWriteBytes += (size_total-size_left);
    pthread_mutex_unlock(&mutex);
    if (size_left != 0) fprintf(stderr, "writeBuffer: diff len: %d %d\n", size_total-size_left, size_total);

    return size_total-size_left;
}

void *Buffer2::staticLogThread(void *arg) {
    Buffer2* buffer = (Buffer2*)arg;
    buffer->logThread();
    return NULL;
}

int Buffer2::getCapacity() {
	return buffer_max;
}

buffer2_statistics_t Buffer2::getStatistics() {
	buffer2_statistics_t stats = this->stats;
	stats.time = Timer::getGlobalTime();

    pthread_mutex_lock(&mutex);
	if (tempBlockingReadTime != -1) {
		stats.blockingReadTime += stats.time - tempBlockingReadTime;
	}
	if (tempBlockingWriteTime != -1) {
		stats.blockingWriteTime += stats.time - tempBlockingWriteTime;
	}
    pthread_mutex_unlock(&mutex);

	return stats;
}

void Buffer2::logThread() {
	BufferLogger* logger = new BufferLogger(logFile);
	logger->logHeader(buffer_max);
	while (!isDestroyed()) {
	    timeval event;
	    gettimeofday(&event, NULL);

		struct timespec time;
		time.tv_sec = event.tv_sec + (int)logInterval;
		time.tv_nsec = event.tv_usec*1000 + (int)((logInterval - floor(logInterval))*1000000000);
		if (time.tv_nsec >= 1000000000) {
			time.tv_nsec -= 1000000000;
			time.tv_sec++;
		}

	    pthread_mutex_lock(&mutex);
        pthread_cond_timedwait(&loggerCond, &mutex, &time);
	    pthread_mutex_unlock(&mutex);

		buffer2_statistics_t stats;
		stats = getStatistics();

		logger->logBuffer(stats);
	}
	delete logger;
}

void Buffer2::setLogFile(string logFile, float interval) {
	this->logFile = logFile;
	this->logInterval = interval;
	if (this->logInterval < 0.1f) {
		this->logInterval = 0.1f; // minimum interval.
	}
	this->isLogging = true;
    int rc = pthread_create(&loggerThread, NULL, staticLogThread, (void *)this);
    if (rc){
        fprintf(stderr, "setLogFile ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
    }
}

