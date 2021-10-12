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

#ifndef BUFFER2_H
#define BUFFER2_H

#include <pthread.h>
#include <string>
using namespace std;


#include "CellsReader.hpp"
#include "CellsWriter.hpp"

struct buffer2_statistics_t {
	float time;
	int bufferUsage;
	int totalReadBytes;
	float blockingReadTime;
	int totalWriteBytes;
	float blockingWriteTime;

	const buffer2_statistics_t operator-(const buffer2_statistics_t &other) const {
		buffer2_statistics_t ret;
		ret.time = this->time - other.time;
		ret.bufferUsage = this->bufferUsage - other.bufferUsage;
		ret.totalReadBytes = this->totalReadBytes - other.totalReadBytes;
		ret.blockingReadTime = this->blockingReadTime - other.blockingReadTime;
		ret.totalWriteBytes = this->totalWriteBytes - other.totalWriteBytes;
		ret.blockingWriteTime = this->blockingWriteTime - other.blockingWriteTime;
		return ret;
	}
};



class Buffer2 {
public:
	Buffer2(int bufferMax);
	virtual ~Buffer2();
	
	int readBuffer(cell_t* data, int nmemb);
	int writeBuffer(const cell_t* data, int nmemb);
	void waitEmptyBuffer();
	
	void destroy();
	bool isDestroyed();

	/**
	 * Returns the maximum capacity in bytes of the buffer.
	 * @return The size of the buffer in bytes.
	 */
	int getCapacity();

	/**
	 * Returns a struct containing statistics of the buffer.
	 * @return statistics of the buffer.
	 */
	buffer2_statistics_t getStatistics();

	/**
	 * Defines a logfile to receive the buffer statistics in each
	 * interval period.
	 *
	 * @param logFile the filename of the log file.
	 * @param interval The period in seconds to log the statistics.
	 */
    void setLogFile(string logFile, float interval);

	private:
		buffer2_statistics_t stats;

        float tempBlockingWriteTime;
        float tempBlockingReadTime;


		pthread_mutex_t mutex;
		pthread_cond_t notFullCond;
		pthread_cond_t notEmptyCond;
		pthread_cond_t emptyCond;
		pthread_cond_t loggerCond;
		
		bool destroyed;
		
		cell_t* buffer;
		int buffer_size;
		int buffer_start;
		int buffer_end;
		int buffer_max;

        pthread_t loggerThread;
        string logFile;
        bool isLogging;
        float logInterval; // in seconds

		int circularSkip(int len);
		int circularLoad(cell_t* dst, int len);
		int circularStore(const cell_t* src, int len);
		int sizeUsed();
		int sizeAvailable();

        static void* staticLogThread(void *arg);

	    void logThread();
};

#endif // BUFFER2_H
