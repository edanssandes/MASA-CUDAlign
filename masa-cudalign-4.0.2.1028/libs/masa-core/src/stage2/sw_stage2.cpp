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

#include <sys/stat.h>
#include <sys/types.h>

#include "../common/Common.hpp"
#include "../common/io/InitialCellsReader.hpp"
#include "../common/io/ReversedCellsReader.hpp"

#define DEBUG (0)

/* The object where the SRA partitions of stage 2 are saved */
static SpecialRowsArea* sraStage2;

/* The partition of the stage 1 where the special rows will be read */
static SpecialRowsPartition* sraPartitionStage1;


static AlignerManager* sw;
static IAligner* aligner;
static const score_params_t* score_params;
static int alignment_id;



static crosspoint_t find_next_crosspoint(AlignerManager* sw, crosspoint_t crosspoint0, crosspoint_t crosspoint1, int alignment_start) {
	crosspoint_t next_crosspoint;
	next_crosspoint.type = -1;

	const int i0 = crosspoint0.i;
	const int j0 = crosspoint0.j;
	const int i1 = crosspoint1.i;
	const int j1 = crosspoint1.j;
	int start_type = crosspoint0.type;

    // TODO analisar casos limitrofes. ex.: (i0,j0)=(1,1) - perfect match
    //int d;
	if (DEBUG) printf ( "partition: (%d,%d)-(%d,%d)\n", i0, j0, i1, j1 );

	/*
	 * TODO Copia do stage3!
	 */
    const int yLen = i1-i0;
	const int xLen = j1-j0; // inclusive baseX
	if (DEBUG) printf ( "(yLen,xLen): %d,%d\n", yLen,xLen );


    {

		SeekableCellsReader* firstRow = new InitialCellsReader(
				start_type == TYPE_GAP_1 ? 0 : score_params->gap_open,
				score_params->gap_ext);
		SeekableCellsReader* firstColumn = new InitialCellsReader(
				start_type == TYPE_GAP_2 ? 0 : score_params->gap_open,
				score_params->gap_ext);

		if (alignment_start == AT_ANYWHERE && (crosspoint0.score <= ( xLen+1 ) *score_params->match)) {
			if (DEBUG) printf ( "GOAL AT ANYWHERE!\n" );
			sw->setGoalScore(crosspoint0.score, AT_ANYWHERE);
//		} else if (alignment_start == AT_SEQUENCE_2 || alignment_start == AT_SEQUENCE_1_OR_2) {
//			sw->setGoalScore(crosspoint0.score, AT_SEQUENCE_1_OR_2);
		} else {
			//sw->setGoalScore(crosspoint0.score, AT_SEQUENCE_2);
			sw->setGoalScore(crosspoint0.score, AT_SEQUENCE_1_OR_2);
		}

		SpecialRowsPartition* sraPartitionStage2 = sraStage2->createPartition(i0, j0, i1, j1);
		sraPartitionStage2->setFirstColumnReader(firstColumn);
		sraPartitionStage2->setFirstRowReader(firstRow);
		sraPartitionStage2->setLastColumnWriter(NULL);
		sraPartitionStage2->setLastRowWriter(NULL);
		sw->setSpecialRowsPartition(sraPartitionStage2);



		/* TODO arrumar outra forma de se fazer essa conversão, mantendo independencia do libmasa */
		int partition_start_type;
		if (start_type == TYPE_GAP_1) {
			partition_start_type = START_TYPE_GAP_H;
		} else if (start_type == TYPE_GAP_2) {
			partition_start_type = START_TYPE_GAP_V;
		} else {
			partition_start_type = START_TYPE_MATCH;

		}

		Partition partition(i0, j0, i1, j1);
		sw->alignPartition(partition, partition_start_type);

		next_crosspoint = sw->getNextCrosspoint();
		printf ( "-> NEXT CROSSPOINT (%d,%d) - %d\n", next_crosspoint.i, next_crosspoint.j, next_crosspoint.score);
		if (!sw->isFoundCrosspoint()) {
			fprintf ( stderr, "ERROR: Backtrace lost! End of matrix reached.\n" );
			exit ( 1 );
		}

		sraStage2->truncatePartition(sraPartitionStage2, next_crosspoint.i, next_crosspoint.j);
		//delete sraPartitionStage2;

		//specialRowWriter->flush(next_crosspoint.i);
		//delete specialRowWriter;
    }


	return next_crosspoint;
}


// requires: getAlignerPool != NULL
int stage2_pool_wait(Job* job, int id, bool must_be_final, bool* is_final = NULL, crosspoint_t* next=NULL) {
	Sequence* seq_vertical = job->getAlignmentParams()->getSequence(0);
	Sequence* seq_horizontal = job->getAlignmentParams()->getSequence(1);

	int count = 0;

	int seq0_len = seq_vertical->getInfo()->getSize();
	int seq1_len = seq_horizontal->getInfo()->getSize();

	int j0 = seq_horizontal->getTrimStart()-1;
	int j1 = seq_horizontal->getTrimEnd();

	crosspoint_t c;
	int final;
	int next_id = id;
	if (!job->getAlignerPool()->isLastNode()) {
		bool alreadyComputed = false;

		do {
			do {
				c = job->getAlignerPool()->receiveCrosspoint(&final);
			} while (must_be_final && !final);

			alreadyComputed = false;
			for (int i=0; i<id; i++) {
				CrosspointsFile* crosspointsFile = new CrosspointsFile(
						job->getCrosspointFile(STAGE_1, i));
				crosspointsFile->loadCrosspoints();
				crosspoint_t c0 = crosspointsFile->front();
				delete crosspointsFile;

				if (c0 == c) {
					alreadyComputed = true;
					if (final) {
						next_id = i;
						if (next != NULL) {
							CrosspointsFile* crosspointsFile = new CrosspointsFile(
									job->getCrosspointFile(STAGE_2, next_id));
							crosspointsFile->loadCrosspoints();
							*next = crosspointsFile->back().reverse(seq1_len, seq0_len);
							if (next->type != TYPE_MATCH) {
								next->score += score_params->gap_open;
							}
							delete crosspointsFile;
						}

					}
				}
			}
		} while (alreadyComputed && !final);
	} else {
		CrosspointsFile* crosspointsFile = new CrosspointsFile(
				job->getCrosspointFile(STAGE_1, 0));
		crosspointsFile->loadCrosspoints();
		c = crosspointsFile->front();
		delete crosspointsFile;

		final = 1;
	}

	if (is_final != NULL) {
		*is_final = final;
	}

	if (c.j < j0 || c.j > j1 ) {
		job->getAlignerPool()->dispatchCrosspoint(c, final);
		printf("Not Me!  (%d,%d,%d,%d)   [%d..%d]\n", c.i, c.j, c.score, c.type, j0, j1);
		if (final && next != NULL) {
			*next = c;
		}
		//exit(1);
		return -1;
	} else if (next_id == id) {
		CrosspointsFile* crosspointsFile = new CrosspointsFile(
				job->getCrosspointFile(STAGE_1, id));
		crosspointsFile->setAutoSave();
		crosspointsFile->write(c);
		crosspointsFile->close();
		delete crosspointsFile;
	}

	return next_id;
}

// requires: getAlignerPool != NULL
void stage2_pool_send(Job* job, crosspoint_t crosspoint_r, int final) {
	Sequence* seq_horizontal = job->getAlignmentParams()->getSequence(1);
	int j0 = seq_horizontal->getTrimStart()-1;

	if (crosspoint_r.type != TYPE_MATCH) {
		// TODO uniformizar o ajuste de score em um único lugar
		crosspoint_r.score -= score_params->gap_open;
	}
	if (!job->getAlignerPool()->isFirstNode()) {
		if (final || crosspoint_r.j == j0) {
			job->getAlignerPool()->dispatchCrosspoint(crosspoint_r, final);
		}
	}
	if (crosspoint_r.type != TYPE_MATCH) {
		// TODO uniformizar o ajuste de score em um único lugar
		crosspoint_r.score += score_params->gap_open;
	}
}

crosspoint_t stage2(Job* job, int id) {
	alignment_id = id;
	//job = _job;
	FILE* stats = job->fopenStatistics(STAGE_2, id);
	job->getAlignmentParams()->printParams(stats);
	fflush(stats);

	aligner = job->aligner;
	score_params = aligner->getScoreParameters();
	sw = new AlignerManager(aligner);


	//aligner = job->aligner;
	//score_params = sw->getScoreParameters();

	bool firstSpecialRowIsGapped = (job->alignment_start == AT_SEQUENCE_2 || job->alignment_start == AT_SEQUENCE_1_AND_2);
	//specialRowReader = new SpecialRowReader(job->getSpecialRowsPath(STAGE_1, 0), score_params, firstSpecialRowIsGapped);


	//job->cells_updates = 0;
	
	int reverse = 1;

	Sequence* seq_vertical = new Sequence(job->getAlignmentParams()->getSequence(reverse));
	Sequence* seq_horizontal = new Sequence(job->getAlignmentParams()->getSequence(1-reverse));

	int i0 = seq_vertical->getTrimStart()-1;
	int i1 = seq_vertical->getTrimEnd();
	int j0 = seq_horizontal->getTrimStart()-1;
	int j1 = seq_horizontal->getTrimEnd();

	int ii0 = seq_vertical->getModifiers()->getTrimStart()-1;
	int ii1 = seq_vertical->getModifiers()->getTrimEnd();
	int jj0 = seq_horizontal->getModifiers()->getTrimStart()-1;
	int jj1 = seq_horizontal->getModifiers()->getTrimEnd();

	if (reverse) {
		seq_vertical->reverse();
		seq_horizontal->reverse();
	}
	//seq_vertical->setPadding(GRID_HEIGHT, '\0');



	int seq0_len = seq_vertical->getInfo()->getSize();
	int seq1_len = seq_horizontal->getInfo()->getSize();

	//SpecialRowsArea* sraStage1 = new SpecialRowsArea(job->getSpecialRowsPath(STAGE_1, 0), 12345678);
	SpecialRowsArea* sraStage1 = job->getSpecialRowsArea(STAGE_1, 0);
	//sraPartitionStage1->setFirstRow(score_params, firstSpecialRowIsGapped);
	//sw->setLastColumnReader(sraPartitionStage1);

	//sraStage2 = new SpecialRowsArea(job->getSpecialRowsPath(STAGE_2, id), 12345678);
	sraStage2 = job->getSpecialRowsArea(STAGE_2, id);

	CrosspointsFile* stage1Crosspoints = new CrosspointsFile(job->getCrosspointFile(STAGE_1, id));
	stage1Crosspoints->loadCrosspoints();
	stage1Crosspoints->reverse(seq1_len, seq0_len);
	crosspoint_t crosspoint = stage1Crosspoints->front();
	crosspoint_t crosspoint_r = crosspoint.reverse(seq0_len, seq1_len);
	delete stage1Crosspoints;

	if ((crosspoint_r.j <= i0 || crosspoint_r.j > i1) && crosspoint_r.j != ii0) {
		delete sw;
		delete seq_horizontal;
		delete seq_vertical;
		//FIXME LEAK?
		return crosspoint_r;
	}

	//sraPartitionStage1 = sraStage1->openPartition(j0, i0, j1, i1);

	sraPartitionStage1 = sraStage1->openPartition(crosspoint_r.i, crosspoint_r.j);
//	if (sraPartitionStage1 == NULL) {
//		// forces full gap test on the matrix edges
//		int i = crosspoint_r.i;
//		int j = crosspoint_r.j;
//		if (crosspoint_r.j == ii0) {
//			j = ii0+1;
//		}
//		if (crosspoint_r.i == jj0) {
//			i = jj0+1;
//		}
//		sraPartitionStage1 = sraStage1->openPartition(i, j);
//	}

	sw->setRecurrenceType(NEEDLEMAN_WUNSCH);
	sw->setBlockPruning(false);
	aligner->clearStatistics();

	if (sraPartitionStage1 != NULL && job->getSRALimit() > 0) {
		int max_size = sraPartitionStage1->getLargestInterval();

		// Necessary if we do not have any special row.
		if (max_size == 0) {
			max_size = seq1_len-crosspoint.j;
		}
		
		//int max_rows = seq0_len-crosspoint.i;
		int max_rows = seq0_len;
		
		int flush_interval = job->getFlushInterval(1);

		sw->setSpecialRowInterval(flush_interval);
		long long special_lines_count = (max_rows/flush_interval);
		fprintf(stats, "Flush Interval: %d\n", flush_interval);
		fprintf(stats, "Special lines: %lld\n", special_lines_count);
		fprintf(stats, "Total size: %lld\n", special_lines_count*max_size*8LL);  // TODO 8
		
	} else {
		sw->setSpecialRowInterval(0);
	}
	fprintf(stats, "Flush limit: %lld\n", job->getSRALimit());
	fprintf(stats, "Predicted Traceback: %s\n", job->predicted_traceback?"YES":"NO");
	


	Timer timer;

	int ev_prepare = timer.createEvent("PREPARE");
    int ev_step = timer.createEvent("STEP");
	int ev_end = timer.createEvent("END");

    timer.init();

	bool check_block_results =  (job->alignment_start == AT_ANYWHERE);

	CrosspointsFile* crosspoints = new CrosspointsFile(job->getCrosspointFile(STAGE_2, id));
	crosspoints->setAutoSave();
	crosspoints->write(crosspoint.i, crosspoint.j, crosspoint.score, crosspoint.type);
	// TODO >0 para SW, (i,j) para NW

	if (crosspoint.type != TYPE_MATCH) {
		crosspoint.score += score_params->gap_open;
		crosspoint_r.score += score_params->gap_open;
	}


	timer.eventRecord(ev_prepare);

	SeekableCellsReader* colReader = NULL;
	SeekableCellsReader* rowReader = NULL;
	bool crossingPartitions = true;
	while(crossingPartitions && sraPartitionStage1 != NULL) {
		//CellsReader* baseRow = new InitialCellsReader();
		//sw->setLastRowReader(baseRow);
		colReader = sraPartitionStage1->getFirstColumnReader();
		rowReader = sraPartitionStage1->getFirstRowReader();
		if (DEBUG) printf("readers: %p %p\n", colReader, rowReader);
		if (DEBUG) printf("crosspoint: %d,%d,%d\n", crosspoint.i, crosspoint.j, crosspoint.score);

		crosspoint_t corner;
		if (sraPartitionStage1 != NULL) {
			crosspoint_t corner_r;
			corner_r.i = sraPartitionStage1->getI0();
			corner_r.j = sraPartitionStage1->getJ0();
			corner = corner_r.reverse(seq1_len, seq0_len);
		} else {
			corner = crosspoint;
		}
		if (DEBUG) {
			printf("Partition: (%d,%d,%d,%d)\n", crosspoint.i, crosspoint.j, corner.i, corner.j);
			printf("seq0: %.60s...\n", seq_vertical->getData());
			printf("seq1: %.60s...\n", seq_horizontal->getData());
		}
		sw->setSequences(seq_vertical, seq_horizontal, crosspoint.i, crosspoint.j, corner.i, corner.j, stats);
		while (1) {
		//while (crosspoint.j > 0 && crosspoint.i > 0) { // <----------- TODO
			crosspoint_t crosspoint_r = crosspoint.reverse(seq0_len, seq1_len);

			if (job->alignment_start == AT_ANYWHERE) {
				if (crosspoint.score <= 0) {
					crossingPartitions = false;
					break;
				}
			}
			if (crosspoint_r.i <= sraPartitionStage1->getI0() || crosspoint_r.j <= sraPartitionStage1->getJ0()) {
				break;
			}

			if (DEBUG) fprintf(stdout, ">> %d %d %d\n", crosspoint.i, crosspoint.j, crosspoint.score);
			if (DEBUG) fprintf(stdout, ">> %d %d %d\n", crosspoint_r.i, crosspoint_r.j, crosspoint.score);

			SpecialRow* row = sraPartitionStage1->nextSpecialRow(crosspoint_r.i, crosspoint_r.j, 128); // TODO verificar esse mínimo
			sw->setLastColumnReader(row);
			if (DEBUG) printf("LastColumnReader: %p\n", row);

			if (colReader != NULL) {
				SeekableCellsReader* col = new ReversedCellsReader(colReader);
				//SeekableCellsReader* col = colReader;
				// FIXME leak
				col->seek(crosspoint_r.i-sraPartitionStage1->getI0()+1); // TODO inserir o +1 byte dentro do arquivo
				sw->setLastRowReader(col);
				if (DEBUG) printf("LastRowReader: %p\n", col);
			} else {
				sw->setLastRowReader(NULL);
			}

			crosspoint_t crosspoint1;
			crosspoint1.i = seq0_len - sraPartitionStage1->getJ0();
			crosspoint1.j = seq1_len - sraPartitionStage1->getReadingRow();


			crosspoint = find_next_crosspoint(sw, crosspoint, crosspoint1, job->alignment_start);
			crosspoints->write(crosspoint);
			if (crosspoint.type != TYPE_MATCH) {
				crosspoint.score += score_params->gap_open;
			}

	//	    crosspoint_t tmp = crosspoint;
	//	    tmp.score += (crosspoint.type == TYPE_MATCH ? 0 : score_params->gap_open);
	//		crosspoints->write(tmp);

			timer.eventRecord(ev_step);
		}
		sw->unsetSequences();

		crosspoint_r = crosspoint.reverse(seq0_len, seq1_len);

		sraPartitionStage1 = sraStage1->openPartition(crosspoint_r.i, crosspoint_r.j);
		//break;
	}


	if (crosspoint.score != 0) {
		crosspoint_t origin_r;
		origin_r.i = j0;
		origin_r.j = i0;
		origin_r.score = 0;
		origin_r.type = TYPE_MATCH;
		crosspoint_t origin = origin_r.reverse(seq1_len, seq0_len);
		printf("ORIGIN: CP: %d %d  OR: %d %d  partition: (%d %d) (%d %d) col: %p/%d  row: %p/%d\n",
				crosspoint_r.i, crosspoint_r.j, origin_r.i, origin_r.j, i0, j0, i1, j1,
				colReader, colReader==NULL?-9:colReader->getType(),
				rowReader, rowReader==NULL?-9:rowReader->getType());

		if ((colReader != NULL && ((colReader->getType() == INIT_WITH_GAPS || colReader->getType() == INIT_WITH_GAPS_OPENED) && crosspoint_r.j == origin_r.j && crosspoint_r.i != origin_r.i))
				|| (rowReader != NULL && ((rowReader->getType() == INIT_WITH_GAPS || rowReader->getType() == INIT_WITH_GAPS_OPENED) && crosspoint_r.i == origin_r.i && crosspoint_r.j != origin_r.j))) {
			crosspoint_r = origin_r;
			crosspoint = origin;

			crosspoints->write(crosspoint);
			printf("ORIGIN!! %d %d   %d %d\n", origin_r.i, origin_r.j, origin.i, origin.j);
		}
	}

//////////

    timer.eventRecord(ev_end);
    crosspoints->close();

//	if (job->getAlignerPool() != NULL) {
//		if (crosspoint_r.type != TYPE_MATCH) {
//			// TODO uniformizar o ajuste de score em um único lugar
//			crosspoint_r.score -= score_params->gap_open;
//		}
//		job->getAlignerPool()->dispatchCrosspoint(crosspoint_r.j, crosspoint_r);
//		if (crosspoint_r.type != TYPE_MATCH) {
//			// TODO uniformizar o ajuste de score em um único lugar
//			crosspoint_r.score += score_params->gap_open;
//		}
//	}

	fprintf(stats, "Stage2 times:\n");
	float diff = timer.printStatistics(stats);
	
	fprintf(stats, "        Total: %.4f\n", diff);
	fprintf(stats, "        Cells: %.4e\n", (double)aligner->getProcessedCells());
	fprintf(stats, "        MCUPS: %.4f\n", aligner->getProcessedCells()/1000000.0f/(diff/1000.0f));
	fprintf(stats, "Millions Cells Updates: %.3f\n", aligner->getProcessedCells()/1000000.0f);

	//aligner->finalize();
	aligner->printStatistics(stats);
    delete crosspoints;
	delete sw;
	delete seq_horizontal;
	delete seq_vertical;
	//delete specialRowReader;
	//delete sraPartitionStage1; // TODO internalizar no sra
	//delete sraStage1;

	job->clearSpecialRowsArea(&sraStage1);


	fclose(stats);

	return crosspoint_r;
}
