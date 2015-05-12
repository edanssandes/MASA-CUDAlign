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

#include "Configs.hpp"

#include "../exceptions/IllegalArgumentException.hpp"

const char* DEFAULT_CONFIGS[] = {
	#include "default.h"
	NULL,
};


#define QUOTE(A) #A
#define STRINGIFY(A) QUOTE(A)

#define DEFAULT_SRA_STRATEGY_GLOBAL 0
#define DEFAULT_SRA_STRATEGY_STAGE1 0
#define DEFAULT_SRA_STRATEGY_STAGE2 0
#define DEFAULT_SRA_STRATEGY_STAGE3 0

#define DEFAULT_SRA_PATH_GLOBAL "work.tmp"
#define DEFAULT_SRA_PATH_STAGE1 ""
#define DEFAULT_SRA_PATH_STAGE2 ""
#define DEFAULT_SRA_PATH_STAGE3 ""



Configs::Configs() {
	static config_option_t options[] = {
			{"[global]", "sra-strategy", &sra_strategy[0],
					STRINGIFY(DEFAULT_SRA_STRATEGY_GLOBAL),
					ConfigParser::parse_int_enum, SRA_STRATEGY_ENUM},
			{"[stage1]", "sra-strategy",    &sra_strategy[1],
					STRINGIFY(DEFAULT_SRA_STRATEGY_STAGE1),
					ConfigParser::parse_int_enum, SRA_STRATEGY_ENUM},
			{"[stage2]", "sra-strategy",    &sra_strategy[2],
					STRINGIFY(DEFAULT_SRA_STRATEGY_STAGE2),
					ConfigParser::parse_int_enum, SRA_STRATEGY_ENUM},
			{"[stage3]", "sra-strategy",    &sra_strategy[3],
					STRINGIFY(DEFAULT_SRA_STRATEGY_STAGE3),
					ConfigParser::parse_int_enum, SRA_STRATEGY_ENUM},

			{"[global]", "sra-compression", &sra_compression[0],
					STRINGIFY(DEFAULT_SRA_COMPRESSION_GLOBAL),
					ConfigParser::parse_int_enum, SRA_COMPRESSION_ENUM},
			{"[stage1]", "sra-compression", &sra_compression[1],
					STRINGIFY(DEFAULT_SRA_COMPRESSION_STAGE1),
					ConfigParser::parse_int_enum, SRA_COMPRESSION_ENUM},
			{"[stage2]", "sra-compression", &sra_compression[2],
					STRINGIFY(DEFAULT_SRA_COMPRESSION_STAGE2),
					ConfigParser::parse_int_enum, SRA_COMPRESSION_ENUM},
			{"[stage3]", "sra-compression", &sra_compression[3],
					STRINGIFY(DEFAULT_SRA_COMPRESSION_STAGE3),
					ConfigParser::parse_int_enum, SRA_COMPRESSION_ENUM},

			{"[global]", "sra-path", &sra_path[0],
					DEFAULT_SRA_PATH_GLOBAL,
					ConfigParser::parse_path, NULL},
			{"[stage1]", "sra-path", &sra_path[1],
					DEFAULT_SRA_PATH_STAGE1,
					ConfigParser::parse_path, NULL},
			{"[stage2]", "sra-path", &sra_path[2],
					DEFAULT_SRA_PATH_STAGE2,
					ConfigParser::parse_path, NULL},
			{"[stage3]", "sra-path", &sra_path[3],
					DEFAULT_SRA_PATH_STAGE3,
					ConfigParser::parse_path, NULL},

			{"[global]", "sra-disk-size", &disk_size[0],
					STRINGIFY(DEFAULT_SRA_DISK_SIZE_GLOBAL),
					ConfigParser::parse_longlong_size, NULL},
			{"[stage1]", "sra-disk-size", &disk_size[1],
					STRINGIFY(DEFAULT_SRA_DISK_SIZE_STAGE1),
					ConfigParser::parse_longlong_size, NULL},
			{"[stage2]", "sra-disk-size", &disk_size[2],
					STRINGIFY(DEFAULT_SRA_DISK_SIZE_STAGE2),
					ConfigParser::parse_longlong_size, NULL},
			{"[stage3]", "sra-disk-size", &disk_size[3],
					STRINGIFY(DEFAULT_SRA_DISK_SIZE_STAGE3),
					ConfigParser::parse_longlong_size, NULL},

			{"[global]", "sra-ram-size", &ram_size[0],
					STRINGIFY(DEFAULT_SRA_RAM_SIZE_GLOBAL),
					ConfigParser::parse_longlong_size, NULL},
			{"[stage1]", "sra-ram-size", &ram_size[1],
					STRINGIFY(DEFAULT_SRA_RAM_SIZE_STAGE1),
					ConfigParser::parse_longlong_size, NULL},
			{"[stage2]", "sra-ram-size", &ram_size[2],
					STRINGIFY(DEFAULT_SRA_RAM_SIZE_STAGE2),
					ConfigParser::parse_longlong_size, NULL},
			{"[stage3]", "sra-ram-size", &ram_size[3],
					STRINGIFY(DEFAULT_SRA_RAM_SIZE_STAGE3),
					ConfigParser::parse_longlong_size, NULL},

			{"[global]", "block-pruning", &block_pruning[0],
					"",
					ConfigParser::parse_bool, NULL},
			{"[stage1]", "block-pruning", &block_pruning[1],
					"",
					ConfigParser::parse_bool, NULL},
			{"[stage2]", "block-pruning", &block_pruning[2],
					"",
					ConfigParser::parse_bool, NULL},
			{"[stage3]", "block-pruning", &block_pruning[3],
					"",
					ConfigParser::parse_bool, NULL},

			{"[stage1]", "pruning-initial-score", &stage1_pruning_initial_score,
					"",
					ConfigParser::parse_int, NULL},

			{"[stage3]", "max-partition-size", &max_partition_size[3],
					"",
					ConfigParser::parse_int, NULL},
			{"[stage4]", "max-partition-size", &max_partition_size[4],
					"",
					ConfigParser::parse_int, NULL},

			{"[stage3]", "max-iterations", &max_iterations[3],
					"",
					ConfigParser::parse_int, NULL},
			{"[stage4]", "max-iterations", &max_iterations[4],
					"",
					ConfigParser::parse_int, NULL},

			{"[stage4]", "execution-type", &stage4_execution_type,
					"",
					ConfigParser::parse_int_enum, STAGE_4_EXECUTION_TYPE_ENUM},

			{"[global]", "work-path", (void*)&work_path,
					"",
					ConfigParser::parse_path, NULL},

			{NULL, NULL, (void*)NULL, "", NULL, NULL}
	};
	parser = new ConfigParser(options);

	for (int i=0; i<7; i++) {
		sra_strategy[i] = CONFIG_NOT_SET;
		sra_compression[i] = CONFIG_NOT_SET;
		disk_size[i] = CONFIG_NOT_SET;
		ram_size[i] = CONFIG_NOT_SET;
		block_pruning[i] = CONFIG_NOT_SET;
		max_iterations[i] = CONFIG_NOT_SET;
		max_partition_size[i] = CONFIG_NOT_SET;
		sra_path[i] = "";
	}
	stage1_pruning_initial_score = CONFIG_NOT_SET;
	stage4_execution_type = CONFIG_NOT_SET;
	work_path = "";

	loadConfigs(DEFAULT_CONFIGS);
}

Configs::~Configs()
{
	// TODO Auto-generated destructor stub
}


void Configs::loadFile(string filename) {
	FILE* file = fopen(filename.c_str(), "rt");
	if (file == NULL) {
		//throw IllegalArgumentException("Configuration File not found.");
		fprintf(stderr, "Configuration File not found. %s.\n", filename.c_str());
		return;
	}
	char line[512];
	int linenum = 0;

	while (fgets(line, sizeof(line), file) != NULL) {
		linenum++;
		try {
			parser->parseLine(line);
		} catch (IllegalArgumentException &e) {
			char msg[512];
			sprintf(msg, "%s: Error in line %d: `%s'.", filename.c_str(), linenum, line);
			throw IllegalArgumentException(e.getErr().c_str(), msg);
		}
	}

	//parser->parseFile(file);
	//parser->printFile(stdout);
}

void Configs::loadConfigs(const char** configs) {
	int linenum = 0;
	while (configs[linenum] != NULL) {
		try {
			parser->parseLine(configs[linenum]);
		} catch (IllegalArgumentException &e) {
			char msg[512];
			sprintf(msg, "Error in config `%s'.", configs[linenum]);
			throw IllegalArgumentException(e.getErr().c_str(), msg);
		}
		linenum++;
	}

	//parser->printFile(stdout);
}

void Configs::printFile(FILE* file) {
	parser->printFile(file, true);
}

int Configs::getSRAStrategy(int stage) {
	if (sra_strategy[stage] == CONFIG_NOT_SET) {
		return sra_strategy[GLOBAL];
	} else {
		return sra_strategy[stage];
	}
}

int Configs::getSRACompression(int stage) {
	if (sra_compression[stage] == CONFIG_NOT_SET) {
		return sra_compression[GLOBAL];
	} else {
		return sra_compression[stage];
	}
}

long long Configs::getSRADiskSize(int stage) {
	if (disk_size[stage] == CONFIG_NOT_SET) {
		return disk_size[GLOBAL];
	} else {
		return disk_size[stage];
	}
}

long long Configs::getSRARamSize(int stage) {
	if (ram_size[stage] == CONFIG_NOT_SET) {
		return ram_size[GLOBAL];
	} else {
		return ram_size[stage];
	}
}

bool Configs::isBlockPruningEnabled(int stage) {
	if (block_pruning[stage] == CONFIG_NOT_SET) {
		return block_pruning[GLOBAL];
	} else {
		return block_pruning[stage];
	}
}

int Configs::getMaxIterations(int stage) {
	return max_iterations[stage];
}

int Configs::getMaxPartitionSize(int stage) {
	return max_partition_size[stage];
}

int Configs::getPruningInitialScore() {
	return stage1_pruning_initial_score;
}

int main_test(int argc, const char** argv) {

	Configs configs;
	configs.loadFile(string(argv[1]));

}
