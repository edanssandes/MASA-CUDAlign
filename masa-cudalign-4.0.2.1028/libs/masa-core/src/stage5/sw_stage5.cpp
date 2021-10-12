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
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "../common/Common.hpp"

#define H_MAX (1*1024)
#define W_MAX (1*1024)

static int h[W_MAX][H_MAX];
static int e[W_MAX][H_MAX];
static int f[W_MAX][H_MAX];



#define DEBUG (0)

static int dna_gap_first;
static int dna_gap_open;
static int dna_gap_ext;
static int dna_match;
static int dna_mismatch;

struct total_score_t {
	int score;
	int matches;
	int mismatches;
	int gapOpen;
	int gapExtensions;

	total_score_t() {
		score = 0;
		matches = 0;
		mismatches = 0;
		gapOpen = 0;
		gapExtensions = 0;
	}
};


// i,j 1-based
static void dot(Alignment* alignment, Sequence *seq0, Sequence *seq1, int i, int j, int type) {
    //int pt;
    const char* s0 = seq0->getData()-1;
    const char* s1 = seq1->getData()-1;
    if (DEBUG) printf("(%5d,%5d) ", seq0->getAbsolutePos(i), seq1->getAbsolutePos(j));
    if (type == 0) {
        if (DEBUG) printf("%c%s%c [                 ]\n", s0[i], s0[i]==s1[j]?"-":" ", s1[j]);
    } else if (type == 1) {
    	int adjust = seq1->isReversed()?0:1;
        if (DEBUG) printf("%c - [ add(1, %7d) ]\n", s0[i], j+adjust);
        alignment->addGapInSeq1(seq1->getAbsolutePos(j+adjust));
    } else if (type == 2) {
    	int adjust = seq0->isReversed()?0:1;
        if (DEBUG) printf("- %c [ add(0, %7d) ]\n", s1[j], i+adjust);
        alignment->addGapInSeq0(seq0->getAbsolutePos(i+adjust));
    }
}

// i0, j0, i1, j1: input as 0 based. Alignment includes (i0,j0) and excludes (i1,j1).
static int sw(Alignment* alignment, Sequence *seq0, Sequence *seq1, int i0, int j0, int i1, int j1, int type_s, int type_e, total_score_t* sum_score) {
    if (i0 == i1) {
    	int sum = (j1-j0)*-dna_gap_ext;
    	if (type_s != TYPE_GAP_1) {
    		sum_score->gapOpen++;
    		sum += -dna_gap_open;
    	}
    	for (int j=j1; j>j0; j--) {
    		dot(alignment, seq0, seq1, i0, j, 2);
    		sum_score->gapExtensions++;
    	}
    	sum_score->score += sum;
    	return sum;
    } else if (j0 == j1) {
    	int sum = (i1-i0)*-dna_gap_ext;
    	if (type_s != TYPE_GAP_2) {
    		sum_score->gapOpen++;
    		sum += -dna_gap_open;
    	}
    	for (int i=i1; i>i0; i--) {
    		dot(alignment, seq0, seq1, i, j0, 1);
    		sum_score->gapExtensions++;
    	}
    	sum_score->score += sum;
    	return sum;
    }


	i0++;
    j0++;

    // TODO conferir
    if (type_e == 0) {
        j1++;
        i1++;
    } else if (type_e == TYPE_GAP_2) {
        //j1++;
    }

    // Now i0,j0,i1,j1 are 1-based

    if (DEBUG) printf("%d %d %d %d %d %d\n", i0, j0, i1, j1, type_s, type_e);

    int seq0_len = i1-i0+1;
    int seq1_len = j1-j0+1;


    const char* s0 = seq0->getData()+(i0-1);
    const char* s1 = seq1->getData()+(j0-1);

    for (int j=1; j<=seq1_len; j++) {
        h[0][j] = -j*dna_gap_ext - dna_gap_open*(type_s!=TYPE_GAP_1);
        e[0][j] = -INF;
    }
    h[0][0] = (type_s!=0?-INF:0);
    e[0][0] = (type_s!=1?-INF:0);

    for (int i=1; i<=seq0_len; i++) {
        h[i][0] = -i*dna_gap_ext - dna_gap_open*(type_s!=TYPE_GAP_2);
        f[i][0] = -INF;
        const char s=s0[i-1];

        int* h0 = h[i];
        int* h1 = h[i-1];
        int* e0 = e[i];
        int* e1 = e[i-1];
        int* f0 = f[i];
        for (int j=1; j<=seq1_len; j++) {
            e0[j] = MAX(h1[j]-dna_gap_first, e1[j]-dna_gap_ext);
            f0[j] = MAX(h0[j-1]-dna_gap_first, f0[j-1]-dna_gap_ext);
            h0[j] = MAX3(h1[j-1]+((s==s1[j-1])?dna_match:dna_mismatch), e0[j], f0[j]);
        }
        //printf("\nj:%4d %c SW: %4d\n", j1-j, s1[j-1], h[j][seq0_len]);
    }

    if (false && DEBUG) {
        printf("%8s:\t.\t", "HEADER");
        for (int j=0; j<=seq1_len; j++) {
            printf("%c(%2d)\t", j==0?'*':s1[j-1], (j+j0-1)%100);
        }
        printf("\n");
        for (int i=0; i<=seq0_len; i++) {
            printf("%8d:\t%c\t", i+i0-1, i==0?'*':s0[i-1]);
            for (int j=0; j<=seq1_len; j++) {
                int v=h[i][j];
                if (v <= -INF) {
                    printf("%5s\t", "INF");
                } else {
                    printf("%5d\t", v);
                }
            }
            printf("\n");
        }
    }


    int sum = 0;
    int i=seq0_len;
    int j=seq1_len;
    int score;
    int c;
    
    score = h[seq0_len][seq1_len];
    if (type_e==0) {
        i--;
        j--;
        c=TYPE_MATCH;
    } else if (type_e==TYPE_GAP_2) {
        score -= dna_gap_open;
        int ss = e[seq0_len][seq1_len];
        if (true || ss >= score) { // TODO validar esse if
            score = ss;
            c=TYPE_GAP_2;
        } else {
            c=TYPE_MATCH;
        }
    } else if (type_e==TYPE_GAP_1) {
        score -= dna_gap_open;
        int ss = f[seq0_len][seq1_len];
        if (true || ss > score) { // TODO validar esse if
            score = ss;
            c=TYPE_GAP_1;
        } else {
            c=TYPE_MATCH;
        }
    }

    if (DEBUG) printf ("Score: %5d %d%d\n", score, type_s, type_e);

    while (i>0 && j>0) {

        int dir;

        int _eh = h[i-1][j]-dna_gap_first;
        int _fh = h[i][j-1]-dna_gap_first;
        int _h11 = h[i-1][j-1]+((s0[i-1]==s1[j-1])?dna_match:dna_mismatch);
        int _h10 = e[i][j];
        int _h01 = f[i][j];
        int _h00 = h[i][j];

        if (c==0) {
            if (_h00 == _h11) {
                dir = 0;
                c=TYPE_MATCH;
            } else if (_h00 == _h10) {
                dir = 1;
                if (_h10==_eh) {
                    c=TYPE_MATCH;
                } else {
                    c=TYPE_GAP_2;
                }
            } else if (_h00 == _h01) {
                dir = 2;
                if (_h01==_fh) {
                    c=TYPE_MATCH;
                } else {
                    c=TYPE_GAP_1;
                }
            }             
        } else if (c==TYPE_GAP_2) {
            dir = 1;
            if (_h10==_eh) {
                c=TYPE_MATCH;
            } else {
                c=TYPE_GAP_2;
            }
        } else if (c==TYPE_GAP_1) {
            dir = 2;
            if (_h01==_fh) {
                c=TYPE_MATCH;
            } else {
                c=TYPE_GAP_1;
            }
        }

        int pt=0;

        dot(alignment, seq0, seq1, i0+(i-1), j0+(j-1), dir);
        if (dir == 0) {
            pt = ((s0[i-1]==s1[j-1])?dna_match:dna_mismatch);
            if (s0[i-1]==s1[j-1]) {
            	sum_score->matches++;
            } else {
            	sum_score->mismatches++;
            }
            i--;
            j--;
        } else if (dir == 1) {
            pt = c==TYPE_MATCH ?-dna_gap_first:-dna_gap_ext;
            if (c == TYPE_MATCH) {
            	sum_score->gapOpen++;
            	sum_score->gapExtensions++;
            } else {
            	sum_score->gapExtensions++;
            }
            i--;
        } else if (dir == 2) {
            pt = c==TYPE_MATCH?-dna_gap_first:-dna_gap_ext;
            if (c == TYPE_MATCH) {
            	sum_score->gapOpen++;
            	sum_score->gapExtensions++;
            } else {
            	sum_score->gapExtensions++;
            }
            j--;
        }
        sum += pt;
        //if (DEBUG) printf("   %2d %5d {c:%d, dir:%d} (%d,%d)\n", pt, sum, c, dir, i, j);

    }
    while (i>0) {
        dot(alignment, seq0, seq1, i0+(i-1), j0+(j-1), 1);
		int pt = -dna_gap_ext;
		i--;
    	sum_score->gapExtensions++;
        c=TYPE_GAP_2;
        sum += pt;
        //if (DEBUG) printf("   %2d %5d {c:%d, dir:%d} (%d,%d)*\n", pt, sum, c, 2, i, j);
    }
    while (j>0) {
        dot(alignment, seq0, seq1, i0+(i-1), j0+(j-1), 2);
		int pt = -dna_gap_ext;
		j--;
    	sum_score->gapExtensions++;
        c=TYPE_GAP_1;
        sum += pt;
        //if (DEBUG) printf("   %2d %5d {c:%d, dir:%d} (%d,%d)*\n", pt, sum, c, 2, i, j);
    }
	if (type_s==TYPE_MATCH && c!=TYPE_MATCH) {
		sum -= (dna_gap_open);
	}
	sum_score->score += sum;
	return sum;
}


int stage5(Job* job, int id) {
	FILE* stats = job->fopenStatistics(STAGE_5, id);
	Sequence* seq0 = job->getAlignmentParams()->getSequence(0);
	Sequence* seq1 = job->getAlignmentParams()->getSequence(1);
	job->getAlignmentParams()->printParams(stats);
	fflush(stats);
	dna_gap_open = -job->getAlignmentParams()->getGapOpen();
	dna_gap_ext  = -job->getAlignmentParams()->getGapExtension();
	dna_match    = job->getAlignmentParams()->getMatch();
	dna_mismatch = job->getAlignmentParams()->getMismatch();
	dna_gap_first = dna_gap_ext + dna_gap_open;

	Timer timer2;

	int ev_step = timer2.createEvent("STEP");
	int ev_start = timer2.createEvent("START");
	int ev_msgs = timer2.createEvent("MSGS");
	int ev_finalize = timer2.createEvent("FINALIZE");
	int ev_write_binary = timer2.createEvent("WRITE_BINARY");
	int ev_end = timer2.createEvent("END");

	timer2.init();

	CrosspointsFile* stage4Crosspoints = new CrosspointsFile(job->getCrosspointFile(STAGE_4, id));
	stage4Crosspoints->loadCrosspoints();

//    if (job->getAlignerPool() != NULL) {
//    	int j0 = seq1->getTrimStart()-1;
//    	int j1 = seq1->getTrimEnd();
//
//    	if (!job->getAlignerPool()->isLastNode() && stage4Crosspoints->back().j >= j1) {
//    		CrosspointsFile* tmp = job->getAlignerPool()->receiveCrosspointFile();
//    		stage4Crosspoints->pop_back();
//    		stage4Crosspoints->insert(stage4Crosspoints->end(), tmp->begin(), tmp->end());
//    		delete tmp;
//    	}
//
//    	if (!job->getAlignerPool()->isFirstNode() && stage4Crosspoints->front().j <= j0) {
//    		job->getAlignerPool()->dispatchCrosspointFile(stage4Crosspoints);
//    		//exit(1);
//    		return -1;
//    	}
//
////    	if (id == seq1->getModifiers()->getTrimStart()) {
////    		CrosspointsFile* tmp = job->getAlignerPool()->receiveCrosspointFile(id);
////    		stage4Crosspoints->insert(stage4Crosspoints->end(), tmp->begin(), tmp->end());
////    	} else if (id_next == seq1->getModifiers()->getTrimEnd()) {
////    		job->getAlignerPool()->dispatchCrosspointFile(id, stage4Crosspoints);
////    	} else {
////    		CrosspointsFile* tmp = job->getAlignerPool()->receiveCrosspointFile(id_next);
////    		stage4Crosspoints->insert(stage4Crosspoints->end(), tmp->begin(), tmp->end());
////    		job->getAlignerPool()->dispatchCrosspointFile(id, stage4Crosspoints);
////    	}
//    }

	timer2.eventRecord(ev_msgs);

	crosspoint_t m0 = stage4Crosspoints->front();
	
	Alignment* alignment = new Alignment(job->getAlignmentParams());

    int partition_id = 1;


    int max_size = stage4Crosspoints->getLargestPartitionSize();
    if (max_size > W_MAX || max_size > H_MAX) {
        fprintf(stderr, "ERROR: MAX SIZE: %d\n", max_size);
        exit(1);
    }
    fprintf(stats, "Largest Block: %d\n", max_size);


	timer2.eventRecord(ev_start);

	
	total_score_t sum_score;
    for (; partition_id<stage4Crosspoints->size(); partition_id++) {
        crosspoint_t m1 = stage4Crosspoints->at(partition_id);

        //if (curr.i == 0 && curr.j == 0) break;

        int sum = sw(alignment, seq0, seq1, m0.i, m0.j, m1.i, m1.j, m0.type, m1.type, &sum_score);
		
        //score += sum;
        if (DEBUG) printf("> SW   %5d/%d\n", sum, sum_score.score);
        if (DEBUG) {
			int goal_diff = (m1.score) - (m0.score);
            if (goal_diff!=sum) {
                fprintf(stderr, "[%s] GOAL DIFF: %8d   SUM: %8d   (%d,%d,%d)-(%d,%d,%d)\n", goal_diff==sum?"OK":"ERROR", goal_diff, sum,
                        m0.i, m0.j, m0.type, m1.i, m1.j, m1.type);
                exit(1);
            }
        }

        if (DEBUG) printf("\n");

        m0 = m1;
    }
    // TODO efetuar um sanity check no score/sum. Esse valor deve ser identico ao stage1.

	timer2.eventRecord(ev_step);
	
    crosspoint_t start = stage4Crosspoints->front();
    crosspoint_t end = stage4Crosspoints->back();
    //dot(&alignment, &job->seq[0], &job->seq[1], last.i, last.j, last.type);

    if (DEBUG) printf("\n");

    fprintf(stats, "(%d,%d)\n", start.type, end.type);
	fprintf(stats, "(%d,%d)->(%d,%d)\n", start.i, start.j, end.i, end.j);

	if (stage4Crosspoints->size() != 1) {
		alignment->setStart(0, seq0->getAbsolutePos(start.i+1));
		alignment->setStart(1, seq1->getAbsolutePos(start.j+1));
		alignment->setEnd(0, seq0->getAbsolutePos(end.i));
		alignment->setEnd(1, seq1->getAbsolutePos(end.j));
	} else {
		alignment->setStart(0, -1);
		alignment->setStart(1, -1);
		alignment->setEnd(0, -1);
		alignment->setEnd(1, -1);
	}


	int correct_score = end.score - start.score; // TODO validar
	if (correct_score != sum_score.score) {
		fprintf(stderr, "stage5: Wrong Alignment Score: %d != %d*.\n", sum_score.score, correct_score);
		exit(1);
	}
	alignment->setRawScore(sum_score.score);
	alignment->setMatches(sum_score.matches);
	alignment->setMismatches(sum_score.mismatches);
	alignment->setGapOpen(sum_score.gapOpen);
	alignment->setGapExtensions(sum_score.gapExtensions);
	if (job->dump_blocks) {
		alignment->setPruningFile(job->dump_pruning_text_filename.c_str());
	}
	alignment->finalize();
	job->setAlignment(alignment);

	timer2.eventRecord(ev_finalize);
	
	AlignmentBinaryFile::write(job->getAlignmentBinaryFile(id), alignment);
	timer2.eventRecord(ev_write_binary);
	
	
	/*alignment->printText(job->alignment_filename);
	timer2.eventRecord(ev_write_text);*/
	
	//alignment.drawAlignment("alignment.svg");
    
    fprintf(stats, "Goal Diff: %d\n", end.score-start.score);
	
	timer2.eventRecord(ev_end);
	
	fprintf(stats, "Stage5 times:\n");
	float diff = timer2.printStatistics(stats);
	
	fprintf(stats, "        total: %.4f\n", diff);
	fclose(stats);
	
	delete stage4Crosspoints;
	return 0;
}
