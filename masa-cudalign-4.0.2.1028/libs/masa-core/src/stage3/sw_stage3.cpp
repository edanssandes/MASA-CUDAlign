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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "../common/Common.hpp"
#include "../common/io/InitialCellsReader.hpp"
#include "../common/io/ReversedCellsReader.hpp"

#define DEBUG (0)

/* The object where the SRA partitions of stage 3 are saved */
static SpecialRowsArea* sraStage3;

/* The partition from where the special rows will be read. Initially, this
 * partition is from stage 2, but then it is updated in the next iteratios.  */
static SpecialRowsPartition* sraPrevPartition;


static AlignerManager* sw;
static IAligner* aligner;
static const score_params_t* score_params;


static crosspoint_t find_next_crosspoint (AlignerManager* sw, const crosspoint_t crosspoint0, const crosspoint_t crosspoint1, const bool mustFindCrosspoint) {


	int start_type = crosspoint0.type;

    int i0r = crosspoint0.i;
    int j0r = crosspoint0.j;
    int i1r = crosspoint1.i;
    int j1r = crosspoint1.j;

	SeekableCellsReader* firstRow = new InitialCellsReader(
			start_type == TYPE_GAP_1 ? 0 : score_params->gap_open,
			score_params->gap_ext);
	SeekableCellsReader* firstColumn = new InitialCellsReader(
			start_type == TYPE_GAP_2 ? 0 : score_params->gap_open,
			score_params->gap_ext);

	SpecialRowsPartition* sraPartitionStage3 = NULL;
	if (sraStage3 != NULL) {
		sraPartitionStage3 = sraStage3->createPartition(i0r, j0r, i1r, j1r);
	}
	sraPartitionStage3->setFirstColumnReader(firstColumn);
	sraPartitionStage3->setFirstRowReader(firstRow);
	sraPartitionStage3->setLastColumnWriter(NULL);
	sraPartitionStage3->setLastRowWriter(NULL);
	sw->setSpecialRowsPartition(sraPartitionStage3);


	if (DEBUG) printf ("partition = (%d,%d:%d) - (%d,%d:%d).\n", crosspoint0.i, crosspoint0.j, crosspoint0.score, crosspoint1.i, crosspoint1.j, crosspoint1.score);

	/* TODO arrumar outra forma de se fazer essa conversão, mantendo independencia do libmasa */
	int partition_start_type;
	if (start_type == TYPE_GAP_1) {
		partition_start_type = START_TYPE_GAP_H;
	} else if (start_type == TYPE_GAP_2) {
		partition_start_type = START_TYPE_GAP_V;
	} else {
		partition_start_type = START_TYPE_MATCH;
	}



	if (mustFindCrosspoint) {
    	sw->setGoalScore(crosspoint0.score, AT_SEQUENCE_1_OR_2);
	} else {
		sw->unsetGoalScore();
	}

	Partition partition(i0r, j0r, i1r, j1r);
    if (DEBUG) printf ( "GOAL: %d\n", crosspoint0.score);

	sw->alignPartition(partition, partition_start_type);

	crosspoint_t next_crosspoint;
	next_crosspoint.type = -1;
	if (mustFindCrosspoint) {
		if ( !sw->isFoundCrosspoint() ) {
				fprintf ( stderr, "ERROR: Backtrace lost! End of matrix reached.\n" );
				exit ( 1 );
		}
		next_crosspoint = sw->getNextCrosspoint();
		if (sraPartitionStage3 != NULL) {
			sraStage3->truncatePartition(sraPartitionStage3, next_crosspoint.i, next_crosspoint.j);
		}
	} else {
		next_crosspoint = crosspoint1;
	}

    return next_crosspoint;
}


void processPartition(crosspoint_t crosspoint0, crosspoint_t crosspoint1,
		int seq0_len, int seq1_len, int reverse,
		CrosspointsFile*& crosspoints) {

	// If we don't need to save more rows, so we do not need to process the last partition (last row)
	bool processLastRow = (sraStage3 != NULL);

	crosspoint_t crosspoint = crosspoint0;

	if (reverse) {
		crosspoint.score = (crosspoint0.score - crosspoint1.score);
	} else {
		crosspoint1.score += (crosspoint1.type == TYPE_MATCH ? 0 : score_params->gap_open);
		crosspoint.score = (crosspoint1.score - crosspoint0.score);
	}
	while (true) { // todo era pra ser goal != -INF?
		if (crosspoint.i == crosspoint1.i) {
			break;
		}
		if (reverse) {
			crosspoint.score += (crosspoint.type == TYPE_MATCH ? 0 : score_params->gap_open);
		}

		if (DEBUG) fprintf(stdout, ">> %d %d %d\n", crosspoint.i, crosspoint.j,
				crosspoint.score);
		if (DEBUG) fprintf(stdout, ">> %08X %08X %d\n", crosspoint.i, crosspoint.j,
				crosspoint.score);

		crosspoint_t crosspointr = crosspoint.reverse(seq0_len, seq1_len);
		SpecialRow* row = sraPrevPartition->nextSpecialRow(crosspointr.i, crosspointr.j,
				128); // TODO verificar esse mínimo

		if (row == NULL) {
			if (DEBUG) printf("No more special Rows.. (?)\n");
			crosspoint = crosspoint1;
			crosspoint.score = 0;
			break;
		}
		sw->setLastColumnReader(row);

		SeekableCellsReader* colReader = sraPrevPartition->getFirstColumnReader();
		SeekableCellsReader* col = new ReversedCellsReader(colReader); // TODO LEAK?
		col->seek(crosspointr.i-sraPrevPartition->getI0()+1); // TODO inserir o +1 byte dentro do arquivo
		sw->setLastRowReader(col);

		const int j1 = seq1_len - sraPrevPartition->getReadingRow();
		crosspoint_t crosspointRow;
		crosspointRow.i = crosspoint1.i;
		crosspointRow.j = j1;
		crosspointRow.score = 0;

	    if ( crosspointRow.j >= crosspoint1.j) {
	    	if (processLastRow) {
	    		if (DEBUG) printf ( "Partition Finished??\n" );
		        //sw->setProcessLastColumnFunction(NULL);
		        crosspoint = find_next_crosspoint(sw, crosspoint, crosspointRow, false);
	    	} else {
	    		if (DEBUG) printf ( "Partition Finished\n" );
				crosspoint = crosspoint1;
				crosspoint.score = 0;
	    	}
			break;
	    } else {
	        //sw->setProcessLastColumnFunction(processLastColumnFunction);
	        crosspoint = find_next_crosspoint(sw, crosspoint, crosspointRow, true);
	    }

		int goal_adj;


		if (reverse) {
			goal_adj = (crosspoint1.score + crosspoint.score);
			//crosspoint.score += (crosspoint.type == TYPE_MATCH ? 0 : score_params->gap_open);
		} else {
			crosspoint.score += (crosspoint.type == TYPE_MATCH ? 0 : score_params->gap_open);
			goal_adj = (crosspoint1.score - crosspoint.score);
		}

		//= crosspoint1.score - crosspoint.score + (crosspoint1.type == 0 ? 0 : score_params->gap_open);
		//static int inv_type[] = {0,2,1};
		//int type_adj = inv_type[crosspoint.type];
	    crosspoint_t tmp = crosspoint;
	    tmp.score = goal_adj;

	    //crosspoints->push_back(tmp);
		crosspoints->write(tmp.i, tmp.j, tmp.score, tmp.type);
	}
}

int reduce_partitions(CrosspointsFile*& crosspointsPrev, CrosspointsFile*& crosspoints,
		Sequence* seq_vertical, Sequence* seq_horizontal, SpecialRowsArea*& sraPrev, int reverse, FILE* stats) {
	int seq0_len = seq_vertical->getInfo()->getSize();
	int seq1_len = seq_horizontal->getInfo()->getSize();

	crosspoint_t m1 = crosspointsPrev->back();
	crosspoint_t m0 = crosspointsPrev->front();
	// FIXME we need to respect the maximum partition size capability of the aligner!
	sw->setSequences(seq_vertical, seq_horizontal, m0.i, m0.j, m1.i, m1.j, stats);
	//printf("*********************** %p, %p, %d,%d, %d, %d", seq_vertical->getData(), seq_horizontal->getData(), m0.i, m0.j, m1.i, m1.j);

	crosspoint_t crosspoint0;
	crosspoint_t crosspoint1;
	int partition_id = 0;
	crosspoint1 = crosspointsPrev->front();
	while (partition_id < crosspointsPrev->size() - 1) {
		partition_id++;
		crosspoint0 = crosspoint1;
		crosspoint1 = crosspointsPrev->at(partition_id);

		//crosspoints->push_back(crosspoint0);
		crosspoints->write(crosspoint0.i, crosspoint0.j, crosspoint0.score,	crosspoint0.type);

		crosspoint_t crosspoint0r = crosspoint1.reverse(seq0_len, seq1_len);
		crosspoint_t crosspoint1r = crosspoint0.reverse(seq0_len, seq1_len);

		printf("(%d,%d)-(%d,%d)\n", crosspoint0r.i, crosspoint0r.j, crosspoint1r.i, crosspoint1r.j);
		if (crosspoint0r.i != crosspoint1r.i && crosspoint0r.j != crosspoint1r.j) {
			sraPrevPartition = sraPrev->openPartition(crosspoint0r.i, crosspoint0r.j, crosspoint1r.i, crosspoint1r.j);
			//sraPrevPartition->setFirstRow(score_params, true);
			//sw->setLastColumnReader(sraPrevPartition);
			if (sraPrevPartition->getRowsCount() > 1) { // 1 = fixed constant first row (rowId = 0).
				// We have special rows in this partition
				processPartition(crosspoint0, crosspoint1, seq0_len, seq1_len, reverse, crosspoints);
			} else {
				// We do not have special rows in this partition
				if (sraStage3 != NULL) {
					// So we create an empty partition and avoid the processPartition call
					sraStage3->createPartition(crosspoint0.i, crosspoint0.j, crosspoint1.i, crosspoint1.j);
				}
			}
		} else {
			if (DEBUG) printf("ignoring full-gap partition (%d,%d)-(%d,%d)\n", crosspoint0r.i, crosspoint0r.j, crosspoint1r.i, crosspoint1r.j);
		}
	}
	crosspoints->write(crosspoint1.i, crosspoint1.j, crosspoint1.score, crosspoint1.type);
	crosspoints->close();
	if (DEBUG) printf("********** %d %d ********\n", crosspointsPrev->size(), crosspoints->size());

	sw->unsetSequences();

	return crosspoints->size();
}

void stage3(Job* job, int id) {
	FILE* stats = job->fopenStatistics(STAGE_3, id);
	job->getAlignmentParams()->printParams(stats);
	fprintf(stats, "Initial VmSize: %d KB\n", getMasaProcessVmSize()/1024);
	fflush(stats);
	aligner = job->aligner;
	score_params = aligner->getScoreParameters();
	sw = new AlignerManager(aligner);



	Sequence* seq_vertical = new Sequence(job->getAlignmentParams()->getSequence(0));
	Sequence* seq_horizontal = new Sequence(job->getAlignmentParams()->getSequence(1));

	int seq0_len = seq_vertical->getInfo()->getSize();
	int seq1_len = seq_horizontal->getInfo()->getSize();
	//int seq0_len = seq_vertical->getLen();
	//int seq1_len = seq_horizontal->getLen();
	//int max_len = seq1_len > seq0_len ? seq1_len : seq0_len;

	// TODO alocar no tamanho certo, e considerando reversões de sequências.
	//h_busBase = (cell_t*)malloc((max_len+1)*sizeof(cell_t));


    Timer timer;

	int ev_prepare = timer.createEvent("PREPARE");
    int ev_step = timer.createEvent("STEP");
	int ev_end = timer.createEvent("END");

    /*int ev_step = timer2.createEvent("STEP");
    int ev_start = timer2.createEvent("START");
    int ev_end = timer2.createEvent("END");
    int ev_copy = timer2.createEvent("COPY");
    int ev_alloc = timer2.createEvent("ALLOC");
    int ev_kernel = timer2.createEvent("KERNEL");
    int ev_writeback = timer2.createEvent("WRITEBACK");*/

    timer.init();

//	sw->setSequences(seq_vertical, seq_horizontal);
//	sw->setFirstColumnSource(true);
//	sw->setFirstRowSource(true);
	//sw->setLastColumnDestination(TO_VECTOR);
	sw->setRecurrenceType(NEEDLEMAN_WUNSCH);
	//sw->setCheckLocation(CHECK_NOWHERE);
	sw->setBlockPruning(false);
	aligner->clearStatistics();



	CrosspointsFile* crosspoints = NULL;

	int start_deep = 0;
    int deep = 0;

	SpecialRowsArea* sraPrev;
	CrosspointsFile* crosspointsPrev;
	sraPrev = job->getSpecialRowsArea(STAGE_2, id);
	//sraPrev = new SpecialRowsArea(job->getSpecialRowsPath(STAGE_2, id), 12345678);
	crosspointsPrev = new CrosspointsFile(job->getCrosspointFile(STAGE_2, id));
	crosspointsPrev->loadCrosspoints();

	int min_interval = 1024;
	bool saveSRA = true;
	float step_sum = 0;

    // TODO calcular com melhor precisao?
	int max_deep = 15;
	//calculate_intervals(intervals, max_deep, seq0_len, seq1_len, job->flush_limit);

	timer.eventRecord(ev_prepare);
    while (deep < 15) {

    	int reverse = deep%2;
    	int flushInterval = job->getFlushInterval(2+deep);
    	deep++;
    	fprintf(stderr, "Deep: %d: Special Row Interval: %d/%d\n", deep, flushInterval, min_interval);

    	crosspointsPrev->reverse(seq1_len, seq0_len);

		float step_diff = timer.eventRecord(ev_step);
		step_sum += step_diff;
		fprintf(stats, " step %2d  crosspoints: %8d   time: %.4f   sum:%.4f\n",
				deep-1, crosspointsPrev->size(), step_diff, step_sum);
		fflush(stats);

		crosspoints = new CrosspointsFile(job->getCrosspointFile(STAGE_3, id, deep));

    	if (deep >= start_deep) {
			crosspoints->setAutoSave();

			//sraStage3 = new SpecialRowsArea(job->getSpecialRowsPath(STAGE_3, id, deep), 12345678);
			sraStage3 = job->getSpecialRowsArea(STAGE_3, id, deep);
	    	if (flushInterval < min_interval) {
	    		sw->setSpecialRowInterval(min_interval);
	    		saveSRA = false;
	    	} else {
	    		sw->setSpecialRowInterval(flushInterval);
	    	}
			if (!saveSRA) {
				sraStage3->setPersistentPartitions(false);
				//sraStage3 = NULL;
			}

	    	reduce_partitions(crosspointsPrev, crosspoints, seq_vertical, seq_horizontal, sraPrev, reverse, stats);

			fprintf(stderr, "Stage3: Crosspoints: %d/%d  Rows: %d/%d %s\n",
					crosspointsPrev->size(), crosspoints->size(),
					sraStage3->getRowsCount(), sraStage3->getPartitionsCount(),
					saveSRA?"":"[DON'T SAVE]");
			if (crosspointsPrev->size() == crosspoints->size()) {
				break;
			}
			//delete sraPrev;

			if (sraStage3->getRowsCount() <= sraStage3->getPartitionsCount()) { // 1 fixed first row (rowId = 0) per partition
				break;
			}
    	}


		delete crosspointsPrev;
		crosspointsPrev = crosspoints;

		job->clearSpecialRowsArea(&sraPrev);
		sraPrev = job->getSpecialRowsArea(STAGE_3, id, deep);

		Sequence* seq_aux = seq_vertical;
		seq_vertical = seq_horizontal;
		seq_horizontal = seq_aux;

		seq_vertical->reverse();
		seq_horizontal->reverse();

		seq0_len = seq_vertical->getInfo()->getSize();
		seq1_len = seq_horizontal->getInfo()->getSize();

//		aligner->finalize();
//		sw->setSequences(seq_vertical, seq_horizontal);
//		aligner->initialize();

	    //crosspoints->write(crosspoint1.i, crosspoint1.j, crosspoint1.score, crosspoint1.type);
	    //break;
    }

    // Deleting previous structures
    delete crosspointsPrev;
    job->clearSpecialRowsArea(&sraPrev);

	fprintf(stderr, "-%d: Special Row Interval: %d\n", deep, job->getFlushInterval(1+deep));

	float step_diff = timer.eventRecord(ev_step);
	step_sum += step_diff;
	fprintf(stats, " step %2d  crosspoints: %8d   time: %.4f   sum:%.4f\n",
			deep, crosspoints->size(), step_diff, step_sum);
	fflush(stats);

    if (deep %2 == 0) {
		crosspoints->reverse(seq0_len, seq1_len);
    }
    CrosspointsFile* crosspointsStage3 = new CrosspointsFile(job->getCrosspointFile(STAGE_3, id));
    crosspointsStage3->assign(crosspoints->begin(), crosspoints->end());
    crosspointsStage3->save();
    crosspointsStage3->close();
    delete crosspointsStage3;

    timer.eventRecord(ev_end);

	fprintf(stats, "Stage3 times:\n");
	float diff = timer.printStatistics(stats);
	
	fprintf(stats, "        Total: %.4f\n", diff);
	fprintf(stats, "        Cells: %.4e\n", (double)aligner->getProcessedCells());
	fprintf(stats, "        MCUPS: %.4f\n", aligner->getProcessedCells()/1000000.0f/(diff/1000.0f));
	fprintf(stats, "Millions Cells Updates: %.3f\n", aligner->getProcessedCells() / 1000000.0f);
	fprintf(stats, " Final VmSize: %d KB\n", getMasaProcessVmSize()/1024);
	
	//aligner->finalize();
	aligner->printStatistics(stats);
	delete crosspoints;

	delete seq_horizontal;
	delete seq_vertical;

	//free(h_busBase);
	//h_busBase = NULL;

	fclose(stats);
}
