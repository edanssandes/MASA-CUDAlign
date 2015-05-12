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

#include "libmasa.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../common/Common.hpp"
#include "../stage1/sw_stage1.h"
#include "../stage2/sw_stage2.h"
#include "../stage3/sw_stage3.h"
#include "../stage4/sw_stage4.h"
#include "../stage5/sw_stage5.h"
#include "../stage6/sw_stage6.h"
//#include "testing/AlignerTester.hpp"
#include "../masanet/MasaNet.hpp"
//#include "../masanet/MasaNetCLI.hpp"
#include "../config.h"

#include <sstream>
using namespace std;


/**
 * Default amount of disk/ram space used for flushing special lines.
 */
//#define DEFAULT_RAM_LIMIT (1*1024*1024*1024LL) // 1G
#define DEFAULT_RAM_LIMIT (0) // none
#define DEFAULT_FLUSH_RAM_STRING "0" // SHOW USAGE
#define DEFAULT_DISK_LIMIT (0) // none
#define DEFAULT_FLUSH_DISK_STRING "0" // SHOW USAGE

/**
 * Fork the maximum number of processes supported by the Aligner.
 */
#define MAX_POSSIBLE_FORK (0)

/**
 * Constant used to represent that no special lines will be flushed.
 */
#define NO_FLUSH (-1)

/**
 * Constant used to execute all stages.
 */
#define ALL_STAGES	(0)

/**
 * The default working directory for temprary data.
 */
#define DEFAULT_WORK_DIRECTORY "./work.tmp"

/**
 *
 */
#define DEFAULT_PHASE_3_SIZE 16
#define DEFAULT_MPS_STRING "16" // SHOW USAGE

/**
 *
 */
#define DEFAULT_MAX_ALIGNMENTS 1
#define DEFAULT_MAX_ALIGNMENTS_STRING "1"


/**
 *
 */
#define DEFAULT_BUFFER_LIMIT	(1024*1024)



/**
 * Only pairwise sequence alignment is supported
 */
#define SEQUENCES_COUNT (2)

/**
 * Optarg: command line parameters.
 * Characters are short options. Hexadecimals are long options.
 */
// General Options
#define ARG_HELP                'h'
#define ARG_WORK_DIR            'd'
#define ARG_CLEAR               'c'
#define ARG_VERBOSE             'v'
/*#define ARG_GPU                 'g'
#define ARG_LIST_GPUS           0x8001
#define ARG_MULTIPLE_GPUS       0x8002
#define ARG_BLOCKS              'b'*/
#define ARG_SPECIAL_ROWS_DIR    0x8003
#define ARG_SHARED_DIR			0x8004
#define ARG_WAIT_PART			0x8005
#define ARG_FORK			    0x8006

// Input Options
#define ARG_TRIM                't'
#define ARG_SPLIT               's'
#define ARG_PART				0x9005
#define ARG_CLEAR_N             0x9006
#define ARG_REVERSE             0x9007
#define ARG_COMPLEMENT          0x9008
#define ARG_REVERSE_COMPLEMENT  0x9009

// Alignment Options
#define ARG_ALIGNMENT_START		0x9101
#define ARG_ALIGNMENT_END		0x9102
#define ARG_ALIGNMENT_EDGES		0x9103


// Execution Options
#define ARG_STAGE_1             '1'
#define ARG_NO_FLUSH            'n'
#define ARG_NO_BLOCK_PRUNING    'p'
#define ARG_DUMP_BLOCKS    		0x1007
#define ARG_DISK_SIZE           0x1008
#define ARG_RAM_SIZE            0x1009
#define ARG_FLUSH_COLUMN        0x1010
#define ARG_LOAD_COLUMN         0x1011
#define ARG_ALIGNMENT_ID		0x1012
#define ARG_MAX_ALIGNMENTS		0x1013

#define ARG_MASANET				0x1014
#define ARG_MASANET_CONNECT		0x1015

#define ARG_STAGE_2             '2'
#define ARG_PREDICTED_TRACEBACK 0x2011

#define ARG_STAGE_3             '3'
#define ARG_MAXIMUM_PARTITION   0x3011
#define ARG_NOT_ORTHOGONAL      0x3012
#define ARG_STAGE_4             '4'
#define ARG_STAGE_5             '5'
#define ARG_STAGE_6             '6'
#define ARG_OUTPUT_FORMAT       0x6013
#define ARG_LIST_FORMATS        0x6014

// Tools Options
#define ARG_DRAW_PRUNING		0x7015
#define ARG_TEST				0x7016


#define TOOL_DRAW_PRUNING		(1)


/**
 * Header
 */
#define MASA_HEADER "\
Linked with MASA - Malleable Architecture for Sequence Aligners - "PACKAGE_VERSION"\n\
University of Brasilia/UnB - Brazil \n\
Copyright (c) 2010-2015 Edans Sandes - License GPLv3\n\
This program comes with ABSOLUTELY NO WARRANTY.\n\
\n"



/**
 * Usage string to be shown in help.
 */
#define USAGE "\
Usage: %s [OPTIONS] [FASTA FILE #1] [FASTA FILE #2]                      \n\
\n\
FASTA FILES:            Supply two sequences in fasta format files.            \n\
\n\
\n\
\033[1mGeneral Options:\033[0m\n\
\n\
-h, --help              Shows this help.\n\
-d, --work-dir=DIR      Directory used to store files produced by the stages.\n\
                           Default: "DEFAULT_WORK_DIRECTORY" \n\
--special-rows-dir=DIR  Directory used to store the special rows produced by\n\
                           the gpu stages. The default is to use a subfolder of\n\
                           the work directory (see --work-dir parameter).\n\
--shared-dir=DIR        Directory used to share data between forked instances.\n\
--wait-part=PART        Process will wait until the conclusion of --part=PART.\n\
-c, --clear             Clears the work directory before any computation. This \n\
                           prevents the continuation of previously interrupted \n\
                           execution.\n\
-v, --verbose=LEVEL     Shows informative output during computation.           \n\
                           0: Silently;\n\
                           1: Only shows error messages;\n\
                           2: (Default) Shows progress and statistics;    \n\
                           3: Gives full output data.\n\
--fork                  Fork many processes in order to optimize performance. \n\
--fork=COUNT            Fork with a limited number of processes.\n\
--fork=W1,W2,...,Wn     Fork with the given weight proportions.\n\
\n\
\n\
\033[1mInput Options:\033[0m\n\
\n\
-t, --trim=I0,I1,J0,J1  Trims sequence #1 from position I0 to I1 (inclusive).  \n\
                           and sequence #2 from position J0 to J1 (inclusive). \n\
                           Zero represents either first and last positions.    \n\
                           This parameter is ignored if used together with the \n\
                           --split parameter. \n\
--split=COUNT           Splits sequence #2 in COUNT equal segments. This       \n\
	                       parameter must be used together with the            \n\
                           --part parameter.                                   \n\
--split=W1,W2,...,Wn    Splits sequence #2 in n segments with weighted         \n\
                           proportions. This parameter must be used            \n\
                           together with the --part parameter.                 \n\
--part=PART             When the --split parameter is used, the sequence #2 is \n\
                           divided in many parts. The --part parameter selects \n\
                           which part will be executed by this process.        \n\
                           If the --load-columns and --flush-columns parameters\n\
                           are not set, then the last column will be saved into\n\
                           a file in the current directory.                    \n\
--clear-n               Remove all 'N' characters on both fasta files.\n\
--reverse=[1|2|both]    Reverse strands of sequence 1, 2 or both.              \n\
--complement=[1|2|both] Generate complement (AT,CG) for sequence 1, 2 or both. \n\
--reverse-complement=[1|2|both] \n\
                        Generate reverse-complement (opposite strand) for      \n\
                           sequence 1, 2 or both. This parameter joins the     \n\
                           --reverse and --complement parameters. \n\
\n\
\033[1mAlignment Type:\033[0m\n\
\n\
--alignment-start=[*|1|2|3|+] \n\
--alignment-end=  [*|1|2|3|+] \n\
--alignment-edges=[*|1|2|3|+][*|1|2|3|+] (start,end)\n\
                        Defines where the alignment can start or end.          \n\
                        -    *: any location.\n\
                        -    1: start/end of sequence 1.                       \n\
                        -    2: start/end of sequence 2.                       \n\
                        -    3: start/end of sequences 1 or 2.                 \n\
                        -    +: start/end of sequences 1 and 2.                \n\
\n\
\033[1mStage Options:\033[0m\n\
\n\
\033[1mStage #1 Options:\033[0m\n\
-1, --stage-1           Executes only the stage #1 of algorithm, i.e., returns \n\
                           the best score and its coordinates. Special rows    \n\
                           are stored in disk to allow the execution of the    \n\
                           subsequent stages.\n\
-n, --no-flush          Do not save special rows. Using this option            \n\
                           in stage #1 will prevent the execution of subsequent\n\
                           phases.\n\
-p, --no-block-pruning  Does not use the block pruning optimization            \n\
\n\
--disk-size=SIZE        Limits the disk/ram size available to the special rows.\n\
--ram-size=SIZE            The SIZE parameter may contain suffix M (e.g., 500M)\n\
                           or G (e.g., 10G). This option is ignored if used\n\
                           together with the --no-flush parameter. \n\
                           Default values: "DEFAULT_FLUSH_RAM_STRING"/"DEFAULT_FLUSH_DISK_STRING".\n\
--flush-column=URL      Store the last column cells in some destination. The   \n\
                           URL is given in some of these formats: \n\
                           file://PATH_TO_FILE \n\
                           socket://0.0.0.0:LISTENING_PORT \n\
--load-column=URL       Loads the first column cells from some destination. The\n\
                           URL is given in some of these formats: \n\
                           file://PATH_TO_FILE \n\
                           socket://HOSTNAME:PORT \n\
--dump-blocks           Saves the result of each block in the alignment file.  \n\
--max-alignments        Maximum number of alignments to return. Default:"DEFAULT_MAX_ALIGNMENTS_STRING".\n\
\n\
\033[1mStage #2 Options:\033[0m\n\
-2, --stage-2           Executes only the stage #2 of algorithm, i.e., returns \n\
                           a list of crosspoints inside the optimal alignment. \n\
                           Special columns are stored in disk to allow the     \n\
                           execution of the subsequent stages. The disk size   \n\
                           available to store the special columns may be       \n\
                           configured using the --disk-space parameter. \n\
\n\
\033[1mStage #3 Options:\033[0m\n\
-3, --stage-3           Executes only the stage #3 of algorithm, i.e., returns \n\
                           a bigger list of crosspoints inside the optimal     \n\
                           alignment.\n\
\n\
\033[1mStage #4 Options:\033[0m\n\
-4, --stage-4           Executes only the stage #4 of algorithm, i.e., given a \n\
                           list of coordinates of the optimal alignment,       \n\
                           increases the number of crosspoint using            \n\
                           Myers and Miller's algorithm, until all the         \n\
                           partitions are smaller than the maximum partition   \n\
                           size.\n\
--maximum-partition=SIZE \n\
                        Defines the maximum partition size allowed as output   \n\
                           of the stage #4. This parameter limits the size of  \n\
                           partitions processed in stage #5. \n\
                           Default Value: "DEFAULT_MPS_STRING" \n\
--not-orthogonal        Does not use the orthogonal execution otimization.     \n\
\n\
\033[1mStage #5 Options:\033[0m\n\
-5, --stage-5           Executes only the stage #5 of algorithm, i.e., given   \n\
                           a list of coordinates of the optimal alignment,     \n\
                           returns the full alignment (as binary output).      \n\
\n\
\033[1mStage #6 Options:\033[0m\n\
-6, --stage-6           Executes only the stage #6 of algorithm, i.e., given   \n\
                           an alignment in binary format, returns the full     \n\
                           alignment in the format defined in with the         \n\
                           --output-format argument.\n\
--output-format=FORMAT  Selects the output format of the full alignment        \n\
                           in stage #6. Possibile formats may be listed with   \n\
                           the --list-formats parameter. \n\
--list-formats          Lists all the possible output formats for stage #6.    \n\
\n\
\n\
"


/**
 * Shows the usage of the command line tool.
 */
static void show_usage(char* program_name, Job* job) {
    printf ( USAGE, program_name );
    IAlignerParameters* params = job->aligner->getParameters();
    params->printUsage();
}

static void print_header(char* aligner_header, FILE* file = NULL) {
	if (aligner_header != NULL) {
		if (file == NULL) {
			fprintf (stdout, "\n\033[1m%s\033[0m\n", aligner_header);
		} else {
			fprintf (file, "\n%s\n", aligner_header);
		}
	}
   fprintf (file == NULL ? stdout : file, MASA_HEADER );
}

static int parse_sequence_flags ( char* optarg, bool* flag ) {
    if ( strcasecmp ( optarg, "none" ) ==0 ) {
        flag[0] = false;
        flag[1] = false;
    } else if ( strcasecmp ( optarg, "1" ) ==0 ) {
        flag[0] = true;
        flag[1] = false;
    } else if ( strcasecmp ( optarg, "2" ) ==0 ) {
        flag[0] = false;
        flag[1] = true;
    } else if ( strcasecmp ( optarg, "both" ) ==0 ) {
        flag[0] = true;
        flag[1] = true;
    } else {
        return 0;
    }
    return 1;
}

static int parse_alignment_flags ( char c, int* flag ) {
    if ( c == '*' ) {
    	*flag = AT_ANYWHERE;
    } else if ( c == '1' ) {
    	*flag = AT_SEQUENCE_1;
    } else if ( c == '2' ) {
    	*flag = AT_SEQUENCE_2;
    } else if ( c == '3' ) {
    	*flag = AT_SEQUENCE_1_OR_2;
    } else if ( c == '+' ) {
    	*flag = AT_SEQUENCE_1_AND_2;
    } else {
        return 0;
    }
    return 1;
}

static int parse_proportions ( char* optarg, int** proportions, char* param_name, char* current_arg ) {
    const int MAX_PROPORTIONS = 32;
	int count = 0;
	for (char* c=optarg; *c; c++) {
		if (*c == ',') continue;
		if (*c < '0' || *c > '9') {
			throw IllegalArgumentException("Only comma separated numbers are allowed (e.g. COUNT or W1,W2,...,Wn).", current_arg);
		}
	}
	if (strchr(optarg, ',') != NULL) {
		char tmparg[strlen(optarg)+1];
		strcpy(tmparg, optarg); // avoid parameter destruction
		char* tok = strtok(tmparg, ",");
	    int* weights = new int[MAX_PROPORTIONS];
		while (tok != NULL && count < MAX_PROPORTIONS) {
			sscanf(tok, "%d", &weights[count]);
			if (weights[count] <= 0) {
				throw IllegalArgumentException("Proportion weights must be positive integers.", current_arg);
			}
			count++;
			tok = strtok(NULL, ",");
		}
		if (count >= MAX_PROPORTIONS) {
			fprintf(stderr, "Error: Maximum proportion weights is %d.\n", MAX_PROPORTIONS-1);
			throw IllegalArgumentException("Too many proportion weights.", current_arg);
		}
		weights[count] = 0;
		*proportions = weights;
	} else {
		sscanf( optarg, "%d", &count);
		if (count <= 0) {
			throw IllegalArgumentException("Argument must be a positive integer.", current_arg);
		}
		*proportions = NULL;
	}
	return count;

}

static long long parse_size(char* optarg, char* current_arg) {
	long long size;
	string str;
	str = optarg;
	if (str == "0") {
		return 0;
	}
	char suffix = str[str.length()-1];
	str[str.length()-1] = 0;
	switch ( suffix ) {
	case 'K':
		size = ( long long ) ( atof ( str.c_str() ) *1024LL );
		break;
	case 'M':
		size = ( long long ) ( atof ( str.c_str() ) *1024*1024LL );
		break;
	case 'G':
		size = ( long long ) ( atof ( str.c_str() ) *1024*1024*1024LL );
		break;
	default:
		throw IllegalArgumentException("Wrong size suffix (use 'K', 'M' or 'G').", current_arg);
	}
	/*if ( size == 0 ) {
		throw IllegalArgumentException("Wrong size limit.", current_arg);
	}*/
	return size;
}

/**
 * Show possible output formats used in stage #6->
 */
static void print_output_formats ( FILE* file=stdout ) {
    fprintf ( file, "Output Formats\n" );
    fprintf ( file, "%10s: %s\n", "NAME", "DESCRIPTION" );
    fprintf ( file, "---------------------------\n" );
    for ( output_format_t* format = stage6_formats; format->name; format++ ) {
        fprintf ( file, "%10s: %s\n", format->name, format->description );
    }
}

/**
 * Load sequence with proper flags.
 */
/*static int getFlags (int clear_n, int reverse, int complement) {

    // Select Flags
    int flags = 0;
    if ( clear_n ) flags = ( flags | FLAG_CLEAR_N );
    if ( reverse ) flags = ( flags | FLAG_REVERSE );
    if ( complement ) flags = ( flags | FLAG_COMPLEMENT );

    return flags;
}*/

static void split_sequences ( Job* _job, int split_step, int split_count, int* weights, int wait_step ) {
    long long int proportions[split_count+1];

    proportions[0] = 0;
    for ( int i=0; i<split_count; i++ ) {
   		proportions[i+1] = proportions[i] + weights[i];
    }
    long long int sum = proportions[split_count];
    for ( int i=0; i<split_count; i++ ) {
        printf ( "split[%d]: %.2f%%\n", i,
        		( proportions[i+1]-proportions[i] ) *100.00/sum );
    }

    /* Process split positions. The 'trim_xx' arguments are overwrited. */
    int seq1_len = _job->getAlignmentParams()->getSequence(1)->getLen();
    int trim_j0 = ( int ) ( ( (( long long int ) seq1_len)*proportions[split_step-1] ) /sum + 1 );
    int trim_j1 = ( int ) ( ( (( long long int ) seq1_len)*proportions[split_step] ) /sum );
    //int trim_j0 = ( int ) ( ( ( long long int ) seq1_len* ( split_step-1 ) ) /split_count + 1 );
    //int trim_j1 = ( int ) ( ( ( long long int ) seq1_len*split_step ) /split_count );
    if ( split_step > 1 && _job->load_column_url == "" ) {
        char str[128];
        sprintf ( str, "file://STEP-%d-%d-%d.tmp",
                  split_step-1, split_count, trim_j0-1 );
        _job->load_column_url = str;
    }
    if ( split_step < split_count && _job->flush_column_url == "" ) {
        char str[128];
        sprintf ( str, "file://STEP-%d-%d-%d.tmp",
                  split_step, split_count, trim_j1 );
        _job->flush_column_url = str;
    }
    _job->getAlignmentParams()->getSequence(1)->trim(trim_j0, trim_j1);

    if (wait_step >= 0) {
    	int wait_id = ( int ) ( ( (( long long int ) seq1_len)*proportions[wait_step] ) /sum );
    	_job->setPoolWaitId(wait_id);
    }

}

/**
 * Fork process for mutigpu execution.
 */
static int fork_multi_process ( int count, Job* _job, const int* weights, int split_step) {
	IAligner* aligner = _job->aligner;
	IAlignerParameters* param = _job->aligner->getParameters();

	if (weights == NULL) {
        fprintf (stderr, "No forked instances allowed.\n");
        exit(1);
	}

	int firstId=-1;
	int lastId=-1;
    long long int proportions[count+1];
    int previous_id[count+1];

    proportions[0] = 0;
    for ( int i=0; i<count; i++ ) {
   		proportions[i+1] = proportions[i] + weights[i];
   		if (weights[i] > 0) {
			if (firstId == -1) {
				firstId = i;
			}
			previous_id[i] = lastId;
			lastId = i;
   		}
    }
    long long int sum = proportions[count];
    for ( int i=0; i<count; i++ ) {
        printf ( "fork[%d%c]: %.2f%%\n", i,
        		(i >= firstId && i <= lastId)?'+':' ',
        		( proportions[i+1]-proportions[i] ) *100.00/sum );
    }
    if (firstId == -1) {
        fprintf (stderr, "No forked instances with valid weight.\n");
        exit(1);
    }


    //int child_pid[gpus];
    bool parent;
    for ( int i=0; i<count; i++ ) {
    	if (weights[i] <= 0) continue;
        int pid = fork();
        if ( pid == 0 ) {
            parent = false;
            param->setForkId(i);
            if ( i > firstId ) {
                char str[128];
                sprintf ( str, "socket://127.0.0.1:%d",
                          7000 + previous_id[i] + split_step*count );
                _job->load_column_url = str;
            }
            if ( i < lastId ) {
                char str[128];
                sprintf ( str, "socket://127.0.0.1:%d",
                          7000 + i  + split_step*count);
                _job->flush_column_url = str;
            }
            //_job->gpu = i;
            break;
        }
        //sleep(5); // FIXME
        fprintf(stderr, "+PID: %d\n", pid);
        //child_pid[i] = pid;
        parent = true;
    }
    if ( parent ) {
        int pid;
        int successful = 1;
        do {
        	int status;
            pid = wait(&status);
            if ( pid == -1 && errno != ECHILD ) {
                perror ( "Error during wait()\n" );
                abort();
            }
            fprintf(stderr, "-PID(%d): %s (%d) %s %d\n", pid,
            		WIFEXITED(status)?"child return code":"abnormal error code",
            		WIFEXITED(status)?(char)WEXITSTATUS(status):status,
            		WIFSIGNALED(status)?"Signalized: ":"-",
            		WIFSIGNALED(status)?WTERMSIG(status):0);
            if (!WIFEXITED(status)) {
            	successful = false;
            }
        } while ( pid > 0 );
        if (successful) {
        	fprintf(stderr, "Processes terminated normally\n");
        } else {
        	fprintf(stderr, "Some process aborted the execution.\n");
        }
        exit(0);
    }

    int seq1_len = _job->getAlignmentParams()->getSequence(1)->getLen();

    int trim_j0 = ( int ) ( ( ( long long int ) seq1_len*proportions[param->getForkId()] ) /sum + 1 );
    int trim_j1 = ( int ) ( ( ( long long int ) seq1_len*proportions[param->getForkId()+1] ) /sum );
    //_job->ram_limit = ( int ) ( ( ( long long int ) _job->ram_limit*weights[param->getForkId()] ) /sum );
    //_job->disk_limit = ( int ) ( ( ( long long int ) _job->disk_limit*weights[param->getForkId()] ) /sum );

    _job->getAlignmentParams()->getSequence(1)->trim(trim_j0, trim_j1);

    return 0;
}

void executeTraceback(Job* _job, Timer* timer, int count, int ev_stage2, int ev_stage3, int ev_stage4, int ev_stage5, int ev_stage6) {
	for (int id = 0; id < count; id++) {
		stage2(_job, id);
		timer->eventRecord(ev_stage2);
		stage3(_job, id);
		timer->eventRecord(ev_stage3);
		stage4(_job, id);
		timer->eventRecord(ev_stage4);
		stage5(_job, id);
		timer->eventRecord(ev_stage5);
		stage6(_job, id);
		timer->eventRecord(ev_stage6);
	}
}

/*
 * Program entry point.
 */
int libmasa_entry_point(int argc, char** argv, IAligner* aligner, char* aligner_header) {
	print_header(aligner_header);

    //configs->printFile(stdout);
    Job* _job = new Job(SEQUENCES_COUNT);
    _job->configs = new Configs();

    //AlignerManager::setAligner(new ExampleAligner());

    AlignmentParams* alignment_params = _job->getAlignmentParams();
    alignment_params->setAlignmentMethod(ALIGNMENT_METHOD_LOCAL);
    //alignment_params->setAffineGapPenalties(-DNA_GAP_OPEN, -DNA_GAP_EXT);
    //alignment_params->setMatchMismatchScores(DNA_MATCH, DNA_MISMATCH);
    const score_params_t* score_params = aligner->getScoreParameters();
    alignment_params->setAffineGapPenalties(-score_params->gap_open, -score_params->gap_ext);
    alignment_params->setMatchMismatchScores(score_params->match, score_params->mismatch);

    /* Default Values */
    int verbosity = 2;
    bool clear_work_directory = false;
    int fork_count = NOT_FORKED_INSTANCE;
    const int* fork_proportions = NULL;
    _job->disk_limit = DEFAULT_DISK_LIMIT;
    _job->ram_limit = DEFAULT_RAM_LIMIT;
	_job->block_pruning = true;
	_job->dump_blocks = false;
    _job->setWorkPath ( DEFAULT_WORK_DIRECTORY );
    _job->stage4_maximum_partition_size = DEFAULT_PHASE_3_SIZE;
    _job->stage4_orthogonal_execution = true;
    _job->stage6_output_format = 0;
    //_job->gpu = DETECT_FASTEST_GPU;
    //_job->blocks = 0;
    _job->flush_column_url = "";
    _job->load_column_url = "";
    _job->alignment_start = AT_ANYWHERE;
    _job->alignment_end = AT_ANYWHERE;
    _job->max_alignments = DEFAULT_MAX_ALIGNMENTS;
	_job->peer_listen_port = -1;
	_job->predicted_traceback = false;
	_job->setBufferLimit(DEFAULT_BUFFER_LIMIT);
    int phase = ALL_STAGES;
	int tool = 0;
    int trim_start[2] = {0, 0};
    int trim_end[2] = {0, 0};
    int split_step = 0;
    int split_count = 0;
    int wait_part = -1;
    int* split_proportions = NULL;
    int alignment_id = 0;
    bool clear_n = false;
    bool reverse_seq[SEQUENCES_COUNT] = {false, false};
    bool complement_seq[SEQUENCES_COUNT] = {false, false};
    char *fasta_file[SEQUENCES_COUNT];

    //_job->alignerParameter = AlignerFactory::createAlignerParameter();
    //_job->alignerParameter = new ExampleParameters();
    _job->aligner = aligner;

    int c;
    //string arg_error = "";

    static struct option long_options[] = {
        // General Options
        {"help",        no_argument,            0, ARG_HELP},
        {"work-dir",    required_argument,      0, ARG_WORK_DIR},
        {"special-rows-dir", required_argument,	0, ARG_SPECIAL_ROWS_DIR},
        {"shared-dir",  required_argument,		0, ARG_SHARED_DIR},
        {"wait-part", 	required_argument,		0, ARG_WAIT_PART},
        {"clear",       no_argument,            0, ARG_CLEAR},
        {"verbose",     required_argument,      0, ARG_VERBOSE},
        /*{"gpu",         required_argument,      0, ARG_GPU},
        {"list-gpus",   no_argument,            0, ARG_LIST_GPUS},
        {"multigpu",    no_argument,            0, ARG_MULTIPLE_GPUS},*/
        //{"blocks",      required_argument,      0, ARG_BLOCKS},
        {"fork",		optional_argument,			0, ARG_FORK},

        // Input Options
        {"trim",        required_argument,      0, ARG_TRIM},
        {"split",       required_argument,      0, ARG_SPLIT},
        {"part",        required_argument,      0, ARG_PART},
        {"clear-n",     no_argument,            0, ARG_CLEAR_N},
        {"reverse",     required_argument,      0, ARG_REVERSE},
        {"complement",  required_argument,      0, ARG_COMPLEMENT},
        {"reverse-complement", required_argument, 0, ARG_REVERSE_COMPLEMENT},

        // Input Options
        {"alignment-start", required_argument,  0, ARG_ALIGNMENT_START},
        {"alignment-end", required_argument,  0, ARG_ALIGNMENT_END},
        {"alignment-edges", required_argument,  0, ARG_ALIGNMENT_EDGES},

        // Execution Options
        {"stage-1",     no_argument,            0, ARG_STAGE_1},
        {"no-flush",    no_argument,            0, ARG_NO_FLUSH},
        {"disk-size",   required_argument,      0, ARG_DISK_SIZE},
        {"ram-size",    required_argument,      0, ARG_RAM_SIZE},
        {"flush-column", required_argument,     0, ARG_FLUSH_COLUMN},
        {"load-column", required_argument,      0, ARG_LOAD_COLUMN},
		{"no-block-pruning", no_argument,		0, ARG_NO_BLOCK_PRUNING},
		{"dump-blocks", no_argument,			0, ARG_DUMP_BLOCKS},
		{"alignment-id", required_argument,		0, ARG_ALIGNMENT_ID},
		{"max-alignments", required_argument,	0, ARG_MAX_ALIGNMENTS},
		// Masanet
        {"masanet", 		optional_argument,     	0, ARG_MASANET},
        {"masanet-connect", required_argument,      0, ARG_MASANET_CONNECT},



        {"stage-2",     no_argument,            0, ARG_STAGE_2},
        {"predicted-traceback",		no_argument,			0, ARG_PREDICTED_TRACEBACK},

        {"stage-3",     no_argument,            0, ARG_STAGE_3},

        {"stage-4",     optional_argument,      0, ARG_STAGE_4},
        {"maximum-partition", required_argument, 0, ARG_MAXIMUM_PARTITION},
        {"not-orthogonal", no_argument,         0, ARG_NOT_ORTHOGONAL},

        {"stage-5",     no_argument,            0, ARG_STAGE_5},

        {"stage-6",     no_argument,            0, ARG_STAGE_6},
        {"output-format", required_argument,    0, ARG_OUTPUT_FORMAT},
        {"list-formats", no_argument,           0, ARG_LIST_FORMATS},

		// Tools Options
		//{"draw-pruning", no_argument,			0, ARG_DRAW_PRUNING},
        {"test",		required_argument,		0, ARG_TEST},

        {0, 0, 0, 0}
    };

    opterr = 0; // prevent the error message from getopt

	try {
		while ( 1 ) {

			/* getopt_long stores the option index here. */
			int option_index = 0;

			c = getopt_long ( argc, argv, ":hd:cv:b:t:s:n123456",
							  long_options, &option_index );
			char* current_arg = argv[optind-1];

			/* Detect the end of the options. */
			if ( c == -1 )
				break;

			//printf("c: %c\n", c);
			switch ( c ) {

			case ARG_HELP:
				show_usage(argv[0], _job);
				exit ( 2 );
				break;
			case ARG_WORK_DIR:
				try {
					_job->setWorkPath( optarg );
				} catch (IllegalArgumentException& err) {
					throw IllegalArgumentException(err.getErr().c_str(), current_arg);
				}
				break;
			case ARG_SPECIAL_ROWS_DIR:
				try {
					_job->setSpecialRowsPath( optarg );
				} catch (IllegalArgumentException& err) {
					throw IllegalArgumentException(err.getErr().c_str(), current_arg);
				}
				break;
			case ARG_SHARED_DIR:
				try {
					_job->setSharedPath( optarg );
				} catch (IllegalArgumentException& err) {
					throw IllegalArgumentException(err.getErr().c_str(), current_arg);
				}
				break;
			case ARG_WAIT_PART:
				try {
					wait_part = atoi ( optarg );
				} catch (IllegalArgumentException& err) {
					throw IllegalArgumentException(err.getErr().c_str(), current_arg);
				}
				break;
			case ARG_CLEAR:
				clear_work_directory = true;
				break;
			case ARG_VERBOSE:
				verbosity = atoi ( optarg );
				if ( verbosity < 0 || verbosity > 2) {
					throw IllegalArgumentException("Verbosity must lie in range [0..2].", current_arg);
				}
				break;
			case ARG_FORK:
				if (optarg == NULL) {
					fork_proportions = aligner->getForkWeights();
					fork_count = 0;
					while (fork_proportions[fork_count] != 0) { // zero-terminated vector
						fork_count++;
				    }
				} else {
					int* weights;
					fork_count = parse_proportions(optarg, &weights, "Fork count", current_arg);
					if (weights == NULL) {
						fork_proportions = aligner->getForkWeights();
					} else {
						fork_proportions = weights;
					}
					/*if (strchr(optarg, '/') != NULL) {
						char* tok = strtok(optarg, "/");
						fork_count = 0;
					    const int MAX_FORK = 8;
					    int* weights = new int[MAX_FORK];
						while (tok != NULL && fork_count < (MAX_FORK-1)) {
							sscanf(tok, "%d", &weights[fork_count++]);
							tok = strtok(NULL, "/");
						}
						weights[fork_count] = 0;
						fork_proportions = weights;
					} else {
						sscanf( optarg, "%d", &fork_count);
						if (fork_count <= 0) {
							throw IllegalArgumentException("Fork count must be positive.", current_arg);
						}
						fork_proportions = aligner->getForkWeights();
					}*/
				}
				break;
			case ARG_TRIM:
				if ( optarg != NULL )  {
					sscanf ( optarg, "%d,%d,%d,%d",
							 &trim_start[0], &trim_end[0], &trim_start[1], &trim_end[1] );
					if ((trim_end[0] > 0 && trim_start[0] >= trim_end[0])
							|| (trim_end[1] > 0 && trim_start[1] >= trim_end[1])) {
						throw IllegalArgumentException("--trim ranges cannot be decreasing.", current_arg);
					}
				}
				break;
			case ARG_SPLIT:
				if ( optarg != NULL )  {
					int* weights;
					split_count = parse_proportions(optarg, &weights, "Split count", current_arg);
					if (weights == NULL) {
						split_proportions = new int[split_count+1];
						for (int i=0; i<split_count; i++) {
							split_proportions[i] = 1;
						}
						split_proportions[split_count] = 0;
					} else {
						split_proportions = weights;
					}

					/*int step;
					int count;
					sscanf ( optarg, "%d/%d", &split_step, &split_count );
					if ( split_step > split_count || split_step <= 0 ) {
						throw IllegalArgumentException("SPLIT_STEP must lie in range [1..SPLIT_COUNT].", current_arg);
					}*/
				} else {
					throw IllegalArgumentException("Inform --split parameters.", current_arg);
				}
				break;
			case ARG_PART:
				if ( optarg != NULL )  {
					sscanf ( optarg, "%d", &split_step );
				} else {
					throw IllegalArgumentException("Inform --part parameter.", current_arg);
				}
				break;
			case ARG_CLEAR_N:
				clear_n = true;
				break;
			case ARG_REVERSE:
				if ( !parse_sequence_flags ( optarg, reverse_seq ) ) {
					throw IllegalArgumentException("Wrong reverse argument. Choose 'none', '1', '2' or 'both'.", current_arg);
				}
				break;
			case ARG_COMPLEMENT:
				if ( !parse_sequence_flags ( optarg, complement_seq ) ) {
					throw IllegalArgumentException("Wrong complement argument. Choose 'none', '1', '2' or 'both'.", current_arg);
				}
				break;
			case ARG_REVERSE_COMPLEMENT:
				if ( !parse_sequence_flags ( optarg, complement_seq ) ) {
					throw IllegalArgumentException("Wrong reverse-complement argument. Choose 'none', '1', '2' or 'both'.", current_arg);
				} else {
					reverse_seq[0] = complement_seq[0];
					reverse_seq[1] = complement_seq[1];
				}
				break;

			case ARG_ALIGNMENT_START:
				if ( !parse_alignment_flags ( optarg[0], &_job->alignment_start ) ) {
					throw IllegalArgumentException("Wrong alignment start argument. Choose '*', '1', '2', '3' or '+'.", current_arg);
				}
				break;
			case ARG_ALIGNMENT_END:
				if ( !parse_alignment_flags ( optarg[0], &_job->alignment_end ) ) {
					throw IllegalArgumentException("Wrong alignment end argument. Choose '*', '1', '2', '3' or '+'.", current_arg);
				}
				break;
			case ARG_ALIGNMENT_EDGES:
				if ( !parse_alignment_flags ( optarg[0], &_job->alignment_start ) ) {
					throw IllegalArgumentException("Wrong alignment start argument. Choose '*', '1', '2', '3' or '+'.", current_arg);
				}
				if ( !parse_alignment_flags ( optarg[1], &_job->alignment_end ) ) {
					throw IllegalArgumentException("Wrong alignment end argument. Choose '*', '1', '2', '3' or '+'.", current_arg);
				}
				break;
			case ARG_STAGE_1:
				phase = STAGE_1;
				break;
			case ARG_NO_FLUSH:
				_job->disk_limit = NO_FLUSH;
				_job->ram_limit = NO_FLUSH;
				break;
			case ARG_NO_BLOCK_PRUNING:
				_job->block_pruning = false;
				break;
			case ARG_DUMP_BLOCKS:
				_job->dump_blocks = true;
				break;
			case ARG_DISK_SIZE:
				if ( _job->disk_limit != NO_FLUSH ) {
					_job->disk_limit = parse_size(optarg, current_arg);
				}
				break;
			case ARG_RAM_SIZE:
				if ( _job->ram_limit != NO_FLUSH ) {
					_job->ram_limit = parse_size(optarg, current_arg);
				}
				break;
			case ARG_FLUSH_COLUMN:
				_job->flush_column_url = optarg;
				_job->block_pruning = false; // TODO
				fprintf(stderr, "Warning: Block Pruning is not compatible with multigpus yet.\n");
				break;
			case ARG_LOAD_COLUMN:
				_job->load_column_url = optarg;
				_job->block_pruning = false; // TODO
				fprintf(stderr, "Warning: Block Pruning is not compatible with multigpus yet.\n");
				break;

			case ARG_ALIGNMENT_ID:
				sscanf ( optarg, "%d", &alignment_id );
				if ( alignment_id < 0 ) {
					throw IllegalArgumentException("Wrong alignment id.", current_arg);
				}
				break;
			case ARG_MAX_ALIGNMENTS:
				sscanf ( optarg, "%d", &_job->max_alignments );
				if (_job->max_alignments < 0) {
					throw IllegalArgumentException("Wrong max alignment.", current_arg);
				}
				break;

			case ARG_MASANET:
				if ( optarg != NULL )  {
					_job->peer_listen_port = atoi ( optarg );
				} else {
		    		_job->peer_listen_port = 0;
				}
				break;
			case ARG_MASANET_CONNECT:
				_job->peer_connect = optarg;
				break;

			case ARG_STAGE_2:
				phase = STAGE_2;
				break;

			case ARG_PREDICTED_TRACEBACK:
				_job->predicted_traceback = true;
				break;

			case ARG_STAGE_3:
				phase = STAGE_3;
				break;
			case ARG_MAXIMUM_PARTITION:
				sscanf ( optarg, "%d", &_job->stage4_maximum_partition_size );
				if (_job->stage4_maximum_partition_size < 1) {
					throw IllegalArgumentException("Maximum partition size too small.", current_arg);
				}
				break;
			case ARG_NOT_ORTHOGONAL:
				_job->stage4_orthogonal_execution = false;
				break;

			case ARG_STAGE_4:
				phase = STAGE_4;
				break;

			case ARG_STAGE_5:
				phase = STAGE_5;
				break;

			case ARG_STAGE_6:
				phase = STAGE_6;
				break;
			case ARG_DRAW_PRUNING:
				tool = TOOL_DRAW_PRUNING;
				break;
			case ARG_LIST_FORMATS:
				print_output_formats();
				exit ( 1 );
				break;
			case ARG_OUTPUT_FORMAT:
				_job->stage6_output_format = -1;
				for ( int id=0; stage6_formats[id].name != NULL; id++ ) {
					if ( strcasecmp ( optarg, stage6_formats[id].name ) ==0 ) {
						_job->stage6_output_format = id;
					}
				}
				if ( _job->stage6_output_format == -1 ) {
					stringstream out;
					out << "Wrong output format: " << optarg << ".";
					throw IllegalArgumentException(out.str().c_str(), current_arg);
				}
				break;
			case ARG_TEST: {
				//AlignerTester* tester = new AlignerTester(aligner);
				//return tester->test(optarg);
				throw IllegalArgumentException("Not Implemented.");
			} break;
			case ':':
				throw IllegalArgumentException("An argument must be supplied to this parameter.", current_arg);
				break;
			case '?': {
				int ret = _job->aligner->getParameters()->processArgument(argc, argv);
				if (ret == ARGUMENT_ERROR_NOT_FOUND) {
					throw IllegalArgumentException("Invalid Option.", current_arg);
				} else if (ret == ARGUMENT_ERROR_NO_OPTION) {
					throw IllegalArgumentException("An argument must be supplied to this parameter.", current_arg);
				} else if (ret > 0) {
					throw IllegalArgumentException("Unhandled argument.", current_arg);
				} else if (ret) {
					throw IllegalArgumentException(_job->aligner->getParameters()->getLastError(), current_arg);
				}
			} break;
			default:
				abort ();
				break;
			}
		}

	    /* Mandatory file names */
	    if (_job->peer_listen_port < 0) {
	    	if (argc - optind == 2 ) {
	    		fasta_file[0] = argv[optind++];
	        	fasta_file[1] = argv[optind++];
	    	} else {
	    		throw IllegalArgumentException("Supply two fasta files.");
	    	}
    	}
    } catch (IllegalArgumentException& err) {
    	fprintf(stderr, "%s", err.what());
    	fprintf(stderr, "See `%s --help' for more information.\n", argv[0]);
    	exit(2);
    }

    if (_job->peer_listen_port >=0 ) {
    	MasaNet* peer = new MasaNet(TYPE_PROCESSING_NODE, "MASA-extension");
    	peer->startServer(_job->peer_listen_port);
    	if (_job->peer_connect.length() > 0) {
    		string address = _job->peer_connect;
    		if (peer->connectToPeer(address, CONNECTION_TYPE_CTRL) == NULL) {
    			printf("MasaNet Connection Error.\n");
    			exit(-1);
    		}
    	}
    	sleep(600);
    }

    /* Loads both sequences */
    /*for (int i=0; i<2; i++) {
    	load_sequence (alignment_params->getSeq(i), clear_n, reverse_seq[i], complement_seq[i], fasta_file[i]);
    }*/


	Timer timer;
	int ev_start  = timer.createEvent("START");
	int ev_seqs   = timer.createEvent("SEQUENCES");
	int ev_init   = timer.createEvent("INIT");
	int ev_stage1 = timer.createEvent("STAGE1");
	int ev_stage2 = timer.createEvent("STAGE2");
	int ev_stage3 = timer.createEvent("STAGE3");
	int ev_stage4 = timer.createEvent("STAGE4");
	int ev_stage5 = timer.createEvent("STAGE5");
	int ev_stage6 = timer.createEvent("STAGE6");

	timer.eventRecord(ev_start);


    for (int i=0; i<2; i++) {
    	SequenceInfo* sequenceInfo = new SequenceInfo();
    	sequenceInfo->setFilename(fasta_file[i]);

    	SequenceModifiers* modifiers = new SequenceModifiers();
    	if (i == 0) {
    		modifiers->setClearN(clear_n);
    	} else {
    		modifiers->setClearN(false);
    	}
	    modifiers->setReverse(reverse_seq[i]);
	    modifiers->setComplement(complement_seq[i]);
	    modifiers->setTrimStart(trim_start[i]);
	    modifiers->setTrimEnd(trim_end[i]);

	    Sequence* sequence = new Sequence(sequenceInfo, modifiers);
	    _job->addSequence(sequence);

	    alignment_params->addSequence(sequence);
    }

    //_job->setSequence(seq0, trim_i0, trim_i1, clear_n, reverse_seq[0], complement_seq[0]);

    /* Fork processes if using multiple gpus */
    if (split_count > 0) {
        split_sequences ( _job, split_step, split_count, split_proportions, wait_part );
    }

	timer.eventRecord(ev_seqs);


    if ( fork_count != NOT_FORKED_INSTANCE) {
        if (phase == ALL_STAGES) {
        	//fprintf(stderr, "Warning: only Stage 1 will be executed in forked processes.\n");
        	//phase = STAGE_1;
        } else if (phase != STAGE_1) {
        	fprintf(stderr, "FATAL: only Stage 1 is supported in forked processes.\n");
        	exit(1);
        }
        /*if (_job->disk_limit != NO_FLUSH || _job->ram_limit != NO_FLUSH) {
        	fprintf(stderr, "Warning: disabling flushing rows in forked processes.\n");
        	_job->disk_limit = NO_FLUSH;
        	_job->ram_limit = NO_FLUSH;
        }*/
        if (_job->block_pruning) {
			fprintf(stderr, "Warning: disabling Block Pruning in forked processes.\n");
			_job->block_pruning = false;
        	//_job->disk_limit = NO_FLUSH;
        	//_job->ram_limit = NO_FLUSH;
        }

        fork_multi_process ( fork_count, _job, fork_proportions, split_step );
    }

    alignment_params->printParams(stdout);

    /* Job initialization */
    if ( !_job->initialize() ) { // TODO throw exception
        fprintf(stderr, "Error during Job initialization\n");
        exit ( 1 );
    }

	FILE* aligner_stats = _job->fopenStatistics(ALIGNER_STATISTICS, 0);
	print_header(aligner_header, aligner_stats);
	fprintf(aligner_stats, "%s", argv[0]);
	for (int i=1; i<argc; i++) {
		fprintf(aligner_stats, " %s", argv[i]);
	}
	fprintf(aligner_stats, "\n\n");
	aligner->printInitialStatistics(aligner_stats);
	fflush(aligner_stats);

	timer.eventRecord(ev_init);


    /* Job Execution */

    if ( phase == ALL_STAGES ) {
        int count = stage1 ( _job );
    	timer.eventRecord(ev_stage1);
    	if (_job->getAlignerPool() == NULL) {
    		executeTraceback(_job, &timer, count, ev_stage2, ev_stage3, ev_stage4, ev_stage5, ev_stage6);
    	} else {
        	fprintf(stderr, "FATAL: Pool aligners is only supported with Stage 1 yet.\n");
        	exit(1);
    	}

    } else if ( phase == STAGE_1 ) {
        stage1 ( _job );
		timer.eventRecord(ev_stage1);
    } else if ( phase == STAGE_2 ) {
        stage2 ( _job, alignment_id );
		timer.eventRecord(ev_stage2);
    } else if ( phase == STAGE_3 ) {
        stage3 ( _job, alignment_id );
		timer.eventRecord(ev_stage3);
    } else if ( phase == STAGE_4 ) {
        stage4 ( _job, alignment_id );
		timer.eventRecord(ev_stage4);
    } else if ( phase == STAGE_5 ) {
        stage5 ( _job, alignment_id );
		timer.eventRecord(ev_stage5);
    } else if ( phase == STAGE_6 ) {
        stage6 ( _job, alignment_id );
		timer.eventRecord(ev_stage6);
    }

	FILE* stats = _job->fopenStatistics(STAGE_GLOBAL, 0);
	double size = ((double)_job->getSequence(0)->getLen())*_job->getSequence(1)->getLen();

	float diff = timer.printStatistics(stats);
	fprintf(stats, "        Total: %.4f\n", diff);
	fprintf(stats, "       Matrix: %.4e\n", size);
	fprintf(stats, "        MCUPS: %.4f\n", size/1000000.0f/(diff/1000.0f));

	fclose(stats);

	aligner->finalize();
	aligner->printFinalStatistics(aligner_stats);
	fclose(aligner_stats);

    /* Job finalization */
    /*if (multiple_fork) {
        wait();
    }*/

	// TODO ugly!
	delete _job->getSequence(0)->getModifiers();
	delete _job->getSequence(1)->getModifiers();
	delete _job->getSequence(0)->getInfo();
	delete _job->getSequence(1)->getInfo();
	delete _job->getSequence(0);
	delete _job->getSequence(1);
	delete _job->configs;
	delete _job;

    exit ( 0 );
}




