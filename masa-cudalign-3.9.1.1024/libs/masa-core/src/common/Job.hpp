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

class Job;

#ifndef _JOB_HPP
#define	_JOB_HPP

#include <string>
#include <vector>
using namespace std;

#include "../libmasa/libmasa.hpp"
#include "SpecialRowReader.hpp"
#include "SpecialRowWriter.hpp"
#include "CrosspointsFile.hpp"
#include "biology/biology.hpp"
#include "sra/SpecialRowsArea.hpp"
#include "configs/Configs.hpp"
#include "AlignerPool.hpp"

#define STAGE_1   (1)
#define STAGE_2   (2)
#define STAGE_3   (3)
#define STAGE_4   (4)
#define STAGE_5   (5)
#define STAGE_6   (6)
#define STAGE_TOOL   (7)
#define STAGE_GLOBAL (0)
#define ALIGNER_STATISTICS (-1)

#define AT_NOWHERE			(-1)
#define AT_ANYWHERE			(0)
#define AT_SEQUENCE_1		(1)
#define AT_SEQUENCE_2		(2)
#define AT_SEQUENCE_1_OR_2	(3)
#define AT_SEQUENCE_1_AND_2	(4)


class Job {
public:
	/* Parameters */

	/*int blocks;
	int gpu;*/
	int alignment_start;
	int alignment_end;
	int max_alignments;
	long long ram_limit;
	long long disk_limit;
	bool block_pruning;
	bool dump_blocks;
	string flush_column_url;
	string load_column_url;
	int predicted_traceback;
	int stage4_maximum_partition_size;
	bool stage4_orthogonal_execution;
	string dump_pruning_text_filename;
	int stage6_output_format;
	string outputBufferLogFile;
	string inputBufferLogFile;

	IAligner* aligner;
	Configs* configs;

	int peer_listen_port;
	string peer_connect;

	/* Statistics */

	//long long cells_updates;




	Job(int sequencesCount);
	virtual ~Job();

	void setWorkPath(string workPath);
	void setSpecialRowsPath(string specialRowsPath);
	void setSharedPath(string sharedPath);
	string getWorkPath();
	int initialize();

	FILE* fopenStatistics(int stage, int id);

	int getSequenceCount() const;
	void addSequence(Sequence* sequence);
	Sequence* getSequence(int index);
	AlignmentParams* getAlignmentParams() const;
	Alignment* getAlignment() const;
	void setAlignment(Alignment* alignment);

	void loadSequenceData(Sequence* sequence);
	string getCrosspointFile(int stage, int id, int deep = -1);
	string getAlignmentBinaryFile(int id);
	string getAlignmentTextFile(int id);

	SpecialRowsArea* getSpecialRowsArea(int stage, int id, int deep = -1);
	void clearSpecialRowsArea(SpecialRowsArea** area);

	long long getSRALimit();
	long long getFlushInterval(int step);
	AlignerPool* getAlignerPool();
	int getPoolWaitId() const;
	void setPoolWaitId(int id);
	int getBufferLimit() const;
	void setBufferLimit(int bufferLimit);

private:
	vector<Sequence*> sequences;
	AlignmentParams* alignment_params;
	Alignment* alignment;
	string statistics_filename;
	string status_filename;
	string info_filename;
	int last_crosspoint_id;
	string work_path;
	string crosspoints_path;
	string special_rows_path;
	int* flushIntervals;
	int maxFlushDeep;
	AlignerPool* alignerPool;
	string pool_shared_path;
	int pool_wait_id;
	int bufferLimit;

	map<string, SpecialRowsArea*> specialRowsAreas;

	string getSpecialRowsPath(int stage, int id, int deep = -1);
	void createPath(string path);
	void initializeWorkPath();
	string resolve_env(string in);
	void clearSpecialRowsAreas();
	void calculateFlushIntervals(int max_deep, long long limit, int seq0_len, int seq1_len);
};

#endif	/* _JOB_HPP */

