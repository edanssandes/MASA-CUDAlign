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

#include "SpecialRowWriter.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../libmasa/libmasa.hpp"

#define DEBUG	(0)


#define MY_MUTEX_INIT
#define MY_MUTEX_DESTROY
#define MY_MUTEX_LOCK
#define MY_MUTEX_UNLOCK

/*
#define MY_MUTEX_INIT		pthread_mutex_init(&mutex, NULL);
#define MY_MUTEX_DESTROY	pthread_mutex_destroy(&mutex);
#define MY_MUTEX_LOCK		pthread_mutex_lock(&mutex);
#define MY_MUTEX_UNLOCK		pthread_mutex_unlock(&mutex);
*/

SpecialRowWriter::SpecialRowWriter(string _directory, int _i0, int _j0, int _j1)
        : directory(_directory), i0(_i0), j0(_j0), j1(_j1) {
    this->element_size = sizeof(cell_t);
    this->lastRow = i0;

    MY_MUTEX_INIT
}

SpecialRowWriter::~SpecialRowWriter() {
	flush();

	MY_MUTEX_DESTROY
}


int SpecialRowWriter::getLastRow() {
	return lastRow;
}

void SpecialRowWriter::flush(int min_i) {
    for (map<int, FILE*>::iterator it = specialRows.begin(); it != specialRows.end(); it++) {
        int i = (*it).first;
        FILE* file = (*it).second;
        if (DEBUG) printf("Unflush: %08X,%08X\n", i, j0);
        fclose(file);

		char old_path[500];
		getFileName(i, true, old_path);
        if (i >= min_i || min_i == -1) {
			remove(old_path);
			if (DEBUG) printf("Removed %s\n", old_path);
        } else {
            char new_path[500];
    	    getFileName(i, false, new_path);
            rename(old_path, new_path);
            // TODO truncate file size from min_j*8 position (create parameter)
        }
    }
    specialRows.clear();
}

void SpecialRowWriter::getFileName(int i, bool temporary, char* str) {
	if (temporary) {
		sprintf(str, "%s/tmp.%08X.%08X", directory.c_str(), i, j0);
	} else {
		sprintf(str, "%s/%08X.%08X", directory.c_str(), i, j0);
	}
}

int SpecialRowWriter::write(int i, void* buf, int len) {
	FILE* file = NULL;
	MY_MUTEX_LOCK
	file = specialRows[i];
	if (file == NULL) {
	    char str[500];
	    getFileName(i, true, str);
	    file = fopen(str, "wb");
	    if (file == NULL) {
	    	fprintf(stderr, "Could not create special row: %s\n", str);
	    	exit(1);
	    }
		specialRows[i] = file;
	}
	MY_MUTEX_UNLOCK

	int ret = fwrite(buf, element_size, len, file);
	if (ret != len) {
    	fprintf(stderr, "Could not write bytes to special row: %d != %d\n", ret, len);
    	perror("Special Row - fwrite");
    	exit(1);
	}

	if (ftell(file) >= abs(j1-j0)*element_size) {
		MY_MUTEX_LOCK
		specialRows.erase(i);
		MY_MUTEX_UNLOCK

        lastRow = i;

        fclose(file);
        file = NULL;

        char old_path[500];
        char new_path[500];
	    getFileName(i, true, old_path);
	    getFileName(i, false, new_path);
        rename(old_path, new_path);
	}

	return ret;
}

/*
mkdir testSRA
rm testSRA/*
g++ -pthread src/common/SpecialRowWriter.cpp -o test && ./test

void* testFunctionThread(void* args) {
	SpecialRowWriter* writer = (SpecialRowWriter*)args;
	int i = (int)((float)rand()/RAND_MAX*100000000);
	int size = 10;
	cell_t data[size];
	float prob = ((float)rand()/RAND_MAX);
	if (prob > 0.90-i*0.2/1000000) {
		sleep(1);
	}
	for (int j=0; j<100000000; j+=size) {
		if (j%1000000 == 0) printf("Write[%d,%d]\n", i, j);
		if (j%100000 == 0) i++;
		writer->write(i, data, size);
	}
}

int main() {
	pthread_t thread[400];
	SpecialRowWriter* writer = new SpecialRowWriter("testSRA/", 0, 0, 1000000);
	for (int i=0; i<100; i++) {
		if (i%5 == 0) sleep(1);
		int rc = pthread_create(&thread[i], NULL, testFunctionThread, (void *)writer);
		printf("pthread_create(thread[%d]): %d\n", i, rc);
	}
	for (int i=0; i<100; i++) {
		pthread_join(thread[i], NULL);
	}
}

*/
