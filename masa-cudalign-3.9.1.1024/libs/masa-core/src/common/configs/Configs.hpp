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

#ifndef CONFIGS_HPP_
#define CONFIGS_HPP_

#include "ConfigParser.hpp"

#include <string>
using namespace std;

#define SRA_STRATEGY_GREEDY		(0)
#define SRA_STRATEGY_AUTO		(1)

#define SRA_STRATEGY_ENUM		"greedy;auto"

#define SRA_COMPRESSION_NONE		(0)
#define SRA_COMPRESSION_8BITS		(1)
#define SRA_COMPRESSION_4BITS 		(2)
#define SRA_COMPRESSION_VLC			(3)
#define SRA_COMPRESSION_2_STEPS		(4)

#define SRA_COMPRESSION_ENUM		"none;8bits;4bits;vlc;vlc-hilo"

#define STAGE_4_EXECUTION_PARALLEL		(0)
#define STAGE_4_EXECUTION_ORTHOGONAL	(1)
#define STAGE_4_EXECUTION_INTERLEAVED	(2)
#define STAGE_4_EXECUTION_TYPE_ENUM		"parallel;orthogonal;interleaved"

// TODO duplicado do Job.hpp
#define GLOBAL	   (0)
#define STAGE_1   (1)
#define STAGE_2   (2)
#define STAGE_3   (3)
#define STAGE_4   (4)
#define STAGE_5   (5)
#define STAGE_6   (6)


#define CONFIG_NOT_SET	(0x80000000)

class Configs {
public:
	Configs();
	virtual ~Configs();

	void loadFile(string filename);
	void loadConfigs(const char** configs);
	void printFile(FILE* file);

	int getSRAStrategy(int stage);
	int getSRACompression(int stage);
	long long getSRADiskSize(int stage);
	long long getSRARamSize(int stage);
	bool isBlockPruningEnabled(int stage);
	int getMaxIterations(int stage);
	int getMaxPartitionSize(int stage);

	int getPruningInitialScore();

private:
	int sra_strategy[7];
	int sra_compression[7];
	long long disk_size[7];
	long long ram_size[7];
	int block_pruning[7];
	int max_iterations[7];
	int max_partition_size[7];

	int stage1_pruning_initial_score;
	int stage4_execution_type;

	/* Paths */
	string sra_path[7];
	string work_path;
	string sequences_path;

	ConfigParser* parser;
};

#endif /* CONFIGS_HPP_ */
