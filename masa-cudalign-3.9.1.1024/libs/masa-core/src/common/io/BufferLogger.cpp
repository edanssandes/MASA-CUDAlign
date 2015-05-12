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

#include "BufferLogger.hpp"

#include <stdlib.h>
#include "../Timer.hpp"
#include "../../libmasa/libmasa.hpp"

BufferLogger::BufferLogger(string file)
{
	prev_stats.time = 0;
	prev_stats.bufferUsage = 0;
	prev_stats.blockingReadTime = 0;
	prev_stats.blockingWriteTime = 0;
	prev_stats.totalReadBytes = 0;
	prev_stats.totalWriteBytes = 0;

	this->file = fopen(file.c_str(), "wt");
	if (this->file == NULL) {
		fprintf(stderr, "Could not create buffer statistics file.\n");
		exit(1);
	}
}

BufferLogger::~BufferLogger()
{
	if (this->file != NULL) {
		fclose(this->file);
	}
}

void BufferLogger::logHeader(int bufferMax) {
	fprintf(file, "#buffer_max=%d\n", bufferMax/sizeof(cell_t));
	fflush(file);
}

void BufferLogger::logBuffer(buffer2_statistics_t &curr_stats) {
	buffer2_statistics_t delta_stats = curr_stats - prev_stats;
	prev_stats = curr_stats;

	float delta = 0;
	float psi_in = 0;
	float psi_out = 0;

	int size = sizeof(cell_t);
	if (delta_stats.time >= 0.001) {
		delta = ((float)delta_stats.bufferUsage/size)/delta_stats.time;
	}
	if (abs(delta_stats.time - delta_stats.blockingReadTime) > 0.000001) {
		psi_in = ((float)delta_stats.totalReadBytes/size)/delta_stats.time*delta_stats.blockingReadTime/(delta_stats.time-delta_stats.blockingReadTime);
	}
	if (abs(delta_stats.time - delta_stats.blockingWriteTime) > 0.000001) {
		psi_out = ((float)delta_stats.totalWriteBytes/size)/delta_stats.time*delta_stats.blockingWriteTime/(delta_stats.time-delta_stats.blockingWriteTime);
	}

	fprintf(file, "%.2f\t%d\t%d\t%d\t%.2f\t%.2f\t%.2f\t%d\t%d\t%d\t%.2f\t%.2f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\n",
			curr_stats.time,  // 1
			curr_stats.bufferUsage/size, // 2
			curr_stats.totalReadBytes/size, curr_stats.totalWriteBytes/size,    // 3 4
			curr_stats.blockingReadTime, curr_stats.blockingWriteTime,	// 5 6
			delta_stats.time, // 7
			delta_stats.bufferUsage/size, // 8
			delta_stats.totalReadBytes/size, delta_stats.totalWriteBytes/size,    // 9 10
			delta_stats.blockingReadTime, delta_stats.blockingWriteTime,	// 11 12
			delta,  psi_in, psi_out, // 13 14 15
			(delta_stats.totalReadBytes/size)/delta_stats.time, // 16
			(delta_stats.totalWriteBytes/size)/delta_stats.time, // 17
			delta+psi_in, delta+psi_out); // 18 19

	fflush(file);
}


