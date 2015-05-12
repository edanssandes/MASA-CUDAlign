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

#include "Job.hpp"

#include <iostream>
#include <sstream>
using namespace std;

#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>
#include <wordexp.h>
#include "Properties.hpp"
#include "SpecialRowWriter.hpp"
#include "exceptions/exceptions.hpp"

#define DEBUG (0)

#define SRA_DECAY (1.0f)

Job::Job(int sequencesCount) {
    this->alignment_params = new AlignmentParams();
    this->alignment = NULL;
    this->alignerPool = NULL;
    //this->alignment = new Alignment(alignment_params);
    this->special_rows_path = "";
    this->aligner = NULL;
    this->flushIntervals = NULL;
    this->maxFlushDeep = 20;
    this->pool_wait_id = -1;
    this->bufferLimit = 0;
}

Job::~Job() {
	//delete alignment;
	delete alignment_params;
	clearSpecialRowsAreas();
}

int Job::initialize() {
    initializeWorkPath();
	SequenceInfo* seq0 = alignment_params->getSequence(0)->getInfo();
	SequenceInfo* seq1 = alignment_params->getSequence(1)->getInfo();

	calculateFlushIntervals(maxFlushDeep, getSRALimit(), seq0->getSize(), seq1->getSize());

    Properties prop;
    if (prop.initialize(this->info_filename.c_str())) {
        string file0 = prop.get_property("seq0");
        string file1 = prop.get_property("seq1");
        int file0_mismatch = (file0.compare(seq0->getDescription()));
        int file1_mismatch = (file1.compare(seq1->getDescription()));
        if (file0_mismatch || file1_mismatch) {
            printf("Sequence mismatch from previous run. Try cleaning work directory (--clean)\n");
        }
        if (file0_mismatch) {
            printf("%s != %s\n", file0.c_str(), seq0->getDescription().c_str());
        }
        if (file1_mismatch) {
            printf("%s != %s\n", file1.c_str(), seq1->getDescription().c_str());
        }
        if (file0_mismatch || file1_mismatch) {
            return 0;  // TODO retornar excecao
        }
    } else {
		FILE* f = fopen(this->info_filename.c_str(), "wt");
        fprintf(f, "seq0=%s\n", seq0->getDescription().c_str());
        fprintf(f, "seq1=%s\n", seq1->getDescription().c_str());
        fclose(f);
    }

    //cout << this->phase << endl;
    //cout << this->progress << endl;
    cout << this->crosspoints_path << endl;
    cout << this->special_rows_path << endl;

    // TODO poderia ficar fora da class Job?
	aligner->initialize();

    if (getAlignerPool() != NULL) {
		int j0 = getAlignmentParams()->getSequence(1)->getTrimStart()-1;
		int j1 = getAlignmentParams()->getSequence(1)->getTrimEnd();
		int jj0 = getAlignmentParams()->getSequence(1)->getModifiers()->getTrimStart()-1;
		int jj1 = getAlignmentParams()->getSequence(1)->getModifiers()->getTrimEnd();

		getAlignerPool()->registerNode(j0, j0==jj0?-1:j0, j1==jj1?-1:j1, flush_column_url);

//		if (/*load_column_url.length() == 0 &&*/ j0 != jj0) {
//			load_column_url = getAlignerPool()->getLoadURL(j0);
//		}
    }

    return 1; // TODO remover quando for retornar excecao
}

void Job::initializeWorkPath() {
    if (this->pool_shared_path.length() == 0) {
    	this->pool_shared_path = work_path + "/shared";
    }

    if (aligner->getParameters()->getForkId() != NOT_FORKED_INSTANCE) {
        createPath(this->work_path);

        char suffix[20];
        sprintf ( suffix, "/FORK.%02d", aligner->getParameters()->getForkId());
        work_path = work_path + "/" + suffix;
    }
    fprintf(stderr, "Work Path: %s\n", work_path.c_str());

	this->dump_pruning_text_filename = work_path + "/pruning_dump.txt";
	this->outputBufferLogFile = work_path + "/outputBuffer.log";
	this->inputBufferLogFile = work_path + "/inputBuffer.log";
    this->crosspoints_path = work_path + "/crosspoints";
    if (this->special_rows_path.length() == 0) {
    	this->special_rows_path = work_path + "/special_rows";
    }
    this->info_filename = work_path + "/info";
    this->status_filename = work_path + "/status";

	createPath(this->work_path);
    createPath(this->crosspoints_path);
    createPath(this->special_rows_path);
    createPath(this->pool_shared_path);
}

void Job::setWorkPath(string workPath) {
    this->work_path = resolve_env(workPath);
}

void Job::setSpecialRowsPath(string specialRowsPath) {
	this->special_rows_path = resolve_env(specialRowsPath);
}

void Job::setSharedPath(string sharedPath) {
    this->pool_shared_path = resolve_env(sharedPath);
}

int Job::getSequenceCount() const {
	return sequences.size();
}

void Job::addSequence(Sequence* sequence) {
	sequences.push_back(sequence);
}

Sequence* Job::getSequence(int index) {
	return sequences[index];
}

AlignmentParams* Job::getAlignmentParams() const {
	return alignment_params;
}

Alignment* Job::getAlignment() const {
	return alignment;
}

void Job::setAlignment(Alignment* alignment) {
	this->alignment = alignment;
}

void Job::loadSequenceData(Sequence* sequence) {
	for (int i=0; i<sequences.size(); i++) {
		if (sequence->getInfo()->isEqual(sequences[i]->getInfo())) {
			sequence->copyData(sequences[i]);
		}
	}
}

string Job::getCrosspointFile(int stage, int id, int deep) {
    char str[500];
    if (deep <= -1) {
    	sprintf(str, "%s/crosspoint_%02d.%02d", crosspoints_path.c_str(), stage, id);
    } else {
    	sprintf(str, "%s/crosspoint_%02d.%02d.r%02d", crosspoints_path.c_str(), stage, id, deep);
    }
    return str;
}

string Job::getSpecialRowsPath(int stage, int id, int deep) {
    char str[500];
    if (deep <= -1) {
    	sprintf(str, "%s/stage.%02d.%02d", special_rows_path.c_str(), stage, id);
    } else {
    	sprintf(str, "%s/stage.%02d.%02d.r%02d", special_rows_path.c_str(), stage, id, deep);
    }
    createPath(str);
    return string(str);
}

void Job::clearSpecialRowsArea(SpecialRowsArea** area) {
	string name = (*area)->getDirectory();
	//printf("Deleting %s: %p\n", name.c_str(), *area);
	specialRowsAreas.erase(name);
	delete *area;
	*area = NULL;
}

AlignerPool* Job::getAlignerPool() {
	if (alignerPool == NULL) {
		if (flush_column_url.length() > 0 || load_column_url.length() > 0) {
			alignerPool = new AlignerPool(pool_shared_path);
			alignerPool->initialize();
		}
	}
	return alignerPool;
}

void Job::calculateFlushIntervals(int max_deep, long long limit, int seq0_len, int seq1_len) {
	if (flushIntervals != NULL) {
		delete flushIntervals;
	}
	flushIntervals = new int[max_deep];
	if (limit < seq1_len*sizeof(cell_t)*2) {
		limit = seq1_len*sizeof(cell_t)*2;
	}
	if (DEBUG) printf("calculate_intervals: limit: %d\n", limit);

	flushIntervals[0] = (int)(((long long)seq0_len)*seq1_len*sizeof(cell_t)/limit + 1);
	flushIntervals[1] = (int)(((long long)flushIntervals[0])*seq1_len*sizeof(cell_t)/(limit/SRA_DECAY) + 1);

	long long interval = flushIntervals[1];

    for (int i=2; i<max_deep; i++) {
    	flushIntervals[i] = (int)(((long long)flushIntervals[i-1])*(i%2==0?seq0_len:seq1_len)*sizeof(cell_t)/(limit/SRA_DECAY) + 1);
        if (flushIntervals[i] > flushIntervals[i-2]/2) {
                flushIntervals[i] = flushIntervals[i-2]/2; // Ensure that each even step decreases at least twice
        }
    }
    if (DEBUG) {
		for (int i=0; i<max_deep; i++) {
			fprintf(stderr, "Interval: %d\n", flushIntervals[i]);
		}
    }
}

long long Job::getFlushInterval(int step) {
	if (step < maxFlushDeep) {
		return flushIntervals[step];
	} else {
		return flushIntervals[maxFlushDeep-1];
	}
}

void Job::clearSpecialRowsAreas() {
	for (map<string, SpecialRowsArea*>::iterator it = specialRowsAreas.begin(); it != specialRowsAreas.end(); it++) {
		//printf("Deleting %s: %p\n", it->second->getDirectory().c_str(), it->second);
		delete it->second;
	}
	specialRowsAreas.clear();
}

SpecialRowsArea* Job::getSpecialRowsArea(int stage, int id, int deep) {
    string name = getSpecialRowsPath(stage, id, deep);
    SpecialRowsArea* area = specialRowsAreas[name];
    if (area == NULL) {
    	createPath(name);
    	long long disk = disk_limit;
    	if (stage == STAGE_2) {
    		disk = (disk + ram_limit)/SRA_DECAY - ram_limit;
    	} else if (stage == STAGE_3) {
    		for (int i=0; i<=deep; i++) {
    			disk = (disk + ram_limit)/SRA_DECAY - ram_limit;
    		}
    	}
    	if (disk < 0) {
    		disk = 0;
    	}
    	printf(" Job::getSpecialRowsArea(%d, %d, %d) -> %d/%d\n", stage, id, deep, ram_limit, disk);
    	area = new SpecialRowsArea(name, ram_limit, disk, aligner->getScoreParameters());
    	specialRowsAreas[name] = area;
    } else {
    	//area->reload();
    }
    return area;
}

string Job::getAlignmentBinaryFile(int id) {
    char str[500];
    sprintf(str, "%s/alignment.%02d.bin", work_path.c_str(), id);
	return str;
}

string Job::getAlignmentTextFile(int id) {
    char str[500];
    sprintf(str, "%s/alignment.%02d.txt", work_path.c_str(), id);
	return str;
}


string Job::getWorkPath() {
    return this->work_path;
}


void Job::createPath(string path) {
	if (mkdir(path.c_str(), 0774)) {
		if (errno != EEXIST) {
			fprintf(stderr, "Path (%s) could not be created. Try to create it manually. (errno: %d)\n", path.c_str(), errno);
			exit(1);
		}
	}
}

FILE* Job::fopenStatistics(int stage, int id) {
	char str[500];
	if (stage == STAGE_GLOBAL) {
		sprintf(str, "%s/statistics", this->work_path.c_str());
	} else if (stage == ALIGNER_STATISTICS) {
		sprintf(str, "%s/statistics.ALIGNER", this->work_path.c_str());
	} else {
		sprintf(str, "%s/statistics_%02d.%02d", this->work_path.c_str(), stage, id);
	}
	FILE * file = fopen(str, "wt");
	if (file == NULL) {
		fprintf(stderr, "Error opening statistics file: %s\n", str);
		exit(1);
	}
	fprintf(file, "###############################\n");
	if (stage == STAGE_GLOBAL) {
		fprintf(file, "#  GLOBAL STATISTICS          #\n", stage, id);
	} else if (stage == ALIGNER_STATISTICS) {
		fprintf(file, "#  ALIGNER INITIALIZATION     #\n", stage, id);
	} else {
		fprintf(file, "#  STATISTICS FOR STAGE #%d.%d  #\n", stage, id);
	}
	fprintf(file, "###############################\n");
	fflush(file);
	return file;
}

long long Job::getSRALimit() {
	if (disk_limit <= 0 && ram_limit <= 0) {
		return 0;
	} else if (disk_limit <= 0) {
		return ram_limit;
	} else if (ram_limit <= 0) {
		return disk_limit;
	} else {
		return disk_limit + ram_limit;
	}
}

string Job::resolve_env(string in) {
	wordexp_t p;
	char **w;
	int i;

	if (in.length() == 0) {
		throw IllegalArgumentException("Empty argument.");
	}

	int ret = wordexp(in.c_str(), &p, WRDE_NOCMD | WRDE_UNDEF);
	std::stringstream var;
	var << "Variable: " << in;
	if (ret == WRDE_BADVAL) {
		throw IllegalArgumentException("An undefined environment variable was referenced.", var.str().c_str());
	} else if (ret != 0) {
		throw IllegalArgumentException("Wrong environment variable expansion.", var.str().c_str());
	} else if (p.we_wordc == 0) {
		throw IllegalArgumentException("No wildcard ('*') expansion.", var.str().c_str());
	} else if (p.we_wordc > 1) {
		throw IllegalArgumentException("Ambiguous wildcard ('*') expansion.", var.str().c_str());
	}
	w = p.we_wordv;
	string out = w[0];
	wordfree(&p);

	return out;
}

int Job::getPoolWaitId() const {
	return pool_wait_id;
}

void Job::setPoolWaitId(int id) {
	pool_wait_id = id;
}

int Job::getBufferLimit() const {
	return bufferLimit;
}

void Job::setBufferLimit(int bufferLimit) {
	this->bufferLimit = bufferLimit;
}
