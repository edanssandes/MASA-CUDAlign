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

#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../common/Common.hpp"

#include "../libmasa/processors/CPUBlockProcessor.hpp"

//#include <cutil_inline.h>

#define H_MAX (2*64*1024)

#define NUM_THREADS 4

#define DEBUG (0)

/*#define dna_match       (1)
#define dna_mismatch    (-3)
#define dna_gap_ext     (2)
#define dna_gap_open    (3)
#define dna_gap_first   (dna_gap_ext+dna_gap_open)*/

static int dna_gap_first;
static int dna_gap_open;
static int dna_gap_ext;
static int dna_match;
static int dna_mismatch;

typedef struct {
    int partition0;
    int partition1;

    int h0[H_MAX];
    int e0[H_MAX];

    int h1[H_MAX];
    int e1[H_MAX];

    cell_t r0[H_MAX];
    cell_t r1[H_MAX];
    cell_t c0[H_MAX];
    cell_t c1[H_MAX];

    Job* job;
    const CrosspointsFile* crosspoints;
    /* OUT */
    crosspoint_t *out_pos;
} split_args_t;

static crosspoint_t split(Sequence *seq0, Sequence *seq1, int i0, int j0, int i1, int j1, 
						int type_s, int type_e, int score_s, int score_e,
						int *h0, int *h1, int *e0, int *e1);
static crosspoint_t ort_split(Sequence *seq0, Sequence *seq1, int i0, int j0, int i1, int j1, 
						int type_s, int type_e, int score_s, int score_e,
						int *h0, int *h1, int *e0, int *e1);
												
static crosspoint_t ort_split_2(Sequence *seq0, Sequence *seq1, int i0, int j0, int i1, int j1,
						int type_s, int type_e, int score_s, int score_e,
						int *h0, int *h1, int *e0, int *e1, cell_t *r0, cell_t *r1, cell_t *c0, cell_t *c1);

static void *split_thread(void *thread_arg) {
    static int inv_type[] = {0,2,1};

    split_args_t* args = (split_args_t*)thread_arg;

    Job* job = args->job;
    const CrosspointsFile* crosspoints = args->crosspoints;
	Sequence* seq0 = job->getAlignmentParams()->getSequence(0);
	Sequence* seq1 = job->getAlignmentParams()->getSequence(1);

    int i0, j0, i1, j1, type0, type1, score0, score1;

    int partition_id = args->partition0;
    i0 = crosspoints->at(partition_id).i;
    j0 = crosspoints->at(partition_id).j;
    type0 = crosspoints->at(partition_id).type;
	score0 = crosspoints->at(partition_id).score;

    float last_percent = 0;
    for (int k=args->partition0+1; k<=args->partition1; k++) {
        float percent = 100.0f*(k-args->partition0)/(args->partition1-args->partition0);
        if (percent > last_percent+25) {
            printf("Split: %5.1f (%6d/%6d)\n", percent , k, crosspoints->size());
            last_percent = percent;
        }
        partition_id++;
        i1 = crosspoints->at(k).i;
        j1 = crosspoints->at(k).j;
        type1 = crosspoints->at(k).type;
        score1 = crosspoints->at(k).score;

        int delta_i = i1-i0;
        int delta_j = j1-j0;

        int inverse = (delta_i < delta_j);
//        if (delta_i == 0) {
//        	if (j0 < j1-job->stage4_maximum_partition_size) {
//        		int len = delta_j/2;
//        		args->out_pos[k].i = i1;
//        		args->out_pos[k].j = j0+len;
//        		args->out_pos[k].type = TYPE_GAP_1;
//        		args->out_pos[k].score = score0 -len*dna_gap_ext - dna_gap_open*(score0!=TYPE_GAP_1);
//        	} else {
//                args->out_pos[k].type = -1;
//        	}
//        } else
        if (delta_i == 0 || delta_j == 0) {
            args->out_pos[k].type = -1;
        } else if (inverse) {
			if (j0 < j1-job->stage4_maximum_partition_size) {
				crosspoint_t out_tmp;
				bool orthogonal = job->stage4_orthogonal_execution;
				if (orthogonal) {
					if (delta_j/2+1 >= H_MAX) {
						fprintf(stderr, "Info: Orthogonal half-partition is too large (%d/2>%d). Disabling orthogonal execution for this partition\n", delta_j, H_MAX);
						orthogonal = false;
					}
				}

                if ( orthogonal ) {
                	//for (int kkk=0; kkk<500; kkk++)
                    out_tmp = ort_split_2 ( seq1, seq0, j0, i0, j1, i1,
                                          inv_type[type0], inv_type[type1], score0, score1,
                                          args->h0, args->h1, args->e0, args->e1, args->r0, args->r1 , args->c0, args->c1);
//                    out_tmp = ort_split ( seq1, seq0, j0, i0, j1, i1,
//                                          inv_type[type0], inv_type[type1], score0, score1,
//                                          args->h0, args->h1, args->e0, args->e1);
                } else {
                    out_tmp = split ( seq1, seq0, j0, i0, j1, i1,
                                      inv_type[type0], inv_type[type1], score0, score1,
                                      args->h0, args->h1, args->e0, args->e1 );
                }
                /*crosspoint_t out_tmp2 = ort_split(&job->seq[1], &job->seq[0], j0, i0, j1, i1,
										   inv_type[start_type], inv_type[prev_type], start_score, prev_score,
										   args->h0, args->h1, args->e0, args->e1);
				if (out_tmp.i != out_tmp2.i || out_tmp.j != out_tmp2.j  || out_tmp.type != out_tmp2.type || out_tmp.score != out_tmp2.score) {
					fprintf(stderr, "WARN: %d %d %d %d   /    %d %d %d %d\n",
						out_tmp.i, out_tmp.j, out_tmp.type, out_tmp.score,
						out_tmp2.i, out_tmp2.j, out_tmp2.type, out_tmp2.score);
				}*/
				args->out_pos[k].i = out_tmp.j;
                args->out_pos[k].j = out_tmp.i;
                args->out_pos[k].type = inv_type[out_tmp.type];
                args->out_pos[k].score = out_tmp.score;
            } else {
                args->out_pos[k].type = -1;
            }
        } else {
			if (i0 < i1-job->stage4_maximum_partition_size) {
				crosspoint_t out_tmp;
				bool orthogonal = job->stage4_orthogonal_execution;
				if (orthogonal) {
					if (delta_i/2+1 >= H_MAX) {
						fprintf(stderr, "Info: Orthogonal half-partition is too large (%d/2>%d). Disabling orthogonal execution for this partition\n", delta_i, H_MAX);
						orthogonal = false;
					}
				}

                if ( orthogonal ) {
                	//for (int kkk=0; kkk<500; kkk++)
                    out_tmp = ort_split_2 ( seq0, seq1, i0, j0, i1, j1,
                                          type0, type1, score0, score1,
                                          args->h0, args->h1, args->e0, args->e1, args->r0, args->r1, args->c0, args->c1 );
//                    out_tmp = ort_split ( seq0, seq1, i0, j0, i1, j1,
//                                          type0, type1, score0, score1,
//                                          args->h0, args->h1, args->e0, args->e1 );
                } else {
                    out_tmp = split ( seq0, seq1, i0, j0, i1, j1,
                                          type0, type1, score0, score1,
                                          args->h0, args->h1, args->e0, args->e1 );
                }
                /*crosspoint_t out_tmp2 = ort_split(&job->seq[0], &job->seq[1], i0, j0, i1, j1,
										 start_type, prev_type, start_score, prev_score,
										 args->h0, args->h1, args->e0, args->e1);
				if (out_tmp.i != out_tmp2.i || out_tmp.j != out_tmp2.j  || out_tmp.type != out_tmp2.type || out_tmp.score != out_tmp2.score) {
					fprintf(stderr, "WARN: %d %d %d %d   /    %d %d %d %d\n",
						out_tmp.i, out_tmp.j, out_tmp.type, out_tmp.score,
						out_tmp2.i, out_tmp2.j, out_tmp2.type, out_tmp2.score);
				}*/
										 
				args->out_pos[k] = out_tmp;
            } else {
                args->out_pos[k].type = -1;
            }
        }

		//printf("%d,%d,%d,%d\n", args->out_pos[k].type, args->out_pos[k].i, args->out_pos[k].j, args->out_pos[k].score);

        type0 = type1;
		score0 = score1;
        i0 = i1;
        j0 = j1;
    }

    pthread_exit(NULL);
    return NULL;
}

static void create_split_thread(split_args_t* args, pthread_t* pthread) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    int rc = pthread_create(pthread, &attr, split_thread, (void *)args);
    if (rc) {
        printf("ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
    }
}

void processBlock(const char* s0, const char* s1, cell_t *row, cell_t *col,
		const int i0, const int j0, const int i1, const int j1,
		const int type_s) {


	int h_tmp = col[0].h;
	for (int i=0; i<i1; i++) {
		int h_next = col[i+1].h;//-i*dna_gap_ext - dna_gap_open*(type_s!=TYPE_GAP_2);
		int f0 = -INF;
		const char s=s0[i];
		//if (DEBUG) printf("%2d/%2d ", h0[0], e0[0]);
		for (int j=0; j<j1; j++) {
			row[j].f = MAX(row[j].h-dna_gap_first, row[j].f-dna_gap_ext);
			f0 = MAX(h_next-dna_gap_first, f0-dna_gap_ext);
			h_next = MAX3(h_tmp+((s==s1[j])?1:-3), row[j].f, f0);
			h_tmp = row[j].h;
			row[j].h = h_next;
			//if (DEBUG) printf("%2d/%2d ", h_next, e0[j]);
		}
		//if (DEBUG) printf("\n");
		//printf("FW: %d/%d\n", i, mid0);
	}
}

cell_t processCol(const char* s0, const char c, int h11, int h10, cell_t *col, const int len) {

	int f0 = -INF;

	//if (DEBUG) printf("%2d/%2d ", h0[0], e0[0]);
	for (int j=0; j<len; j++) {
		col[j].e = MAX(col[j].h-dna_gap_first, col[j].e-dna_gap_ext);
		f0 = MAX(h10-dna_gap_first, f0-dna_gap_ext);
		h10 = MAX3(h11+((c==s0[j])?dna_match:dna_mismatch), col[j].e, f0);
		h11 = col[j].h;
		col[j].h = h10;
		//if (DEBUG) printf("%2d/%2d ", h_next, e0[j]);
	}
	cell_t cell;
	cell.f = f0;
	cell.h = h10;
	return cell;
	//if (DEBUG) printf("\n");
	//printf("FW: %d/%d\n", i, mid0);
}

static bool match(cell_t a, cell_t b, int diff, crosspoint_t* pt) {
	int sum_match = a.h + b.h;
	int sum_gap = a.f + b.f + dna_gap_open;
	if (DEBUG) printf("[%4d/%4d] [%4d/%4d] = [%4d/%4d] %d\n", a.h, a.f, b.h, b.f, sum_match, sum_gap, diff);

	if (sum_match == diff) {
		pt->type = TYPE_MATCH;
		pt->score = a.h;
		return true;
	} else if (sum_gap == diff) {
		pt->type = TYPE_GAP_2;
		pt->score = a.f;
		return true;
	} else if (sum_match > diff || sum_gap > diff) {
		printf("Error Match\n");
		exit(1);
	} else {
		return false;
	}
}


static crosspoint_t ort_split_2(Sequence *seq0, Sequence *seq1, int i0, int j0, int i1, int j1,
						int type_s, int type_e, int score_s, int score_e,
						int *h0, int *h1, int *e0, int *e1, cell_t *r0, cell_t *r1, cell_t *c0, cell_t *c1) {

	if (DEBUG) printf("%d %d %d %d %d %d   %d %d\n", i0, j0, i1, j1, type_s, type_e, score_s, score_e);

	int seq0_len = i1-i0;
	int seq1_len = j1-j0;

	if (seq1_len >= H_MAX) {
		fprintf(stderr, "Partition size is too large (%d > %d). You need to store more special rows in stages 1-3.\n", seq1_len, H_MAX);
		exit(1);
	}

	int diff = 	(score_e + (type_e == TYPE_MATCH ? 0 : dna_gap_open)*0) - score_s;

	int imid = seq0_len/2;
	int imid0 = imid;
	int imid1 = seq0_len - imid; // (imid1 >= imid0) is always true

	int jmid = seq1_len/2;
	int jmid0 = jmid;
	int jmid1 = seq1_len - jmid; //  (jmid1 >= jmid0) is always true

	/* Forward */

	const char* s0 = seq0->getData(false)+(i0);
	const char* s1 = seq1->getData(false)+(j0);
	const char* s0r = seq0->getData(true)+(seq0->getInfo()->getSize()-i1);
	const char* s1r = seq1->getData(true)+(seq1->getInfo()->getSize()-j1);

//	for (int i=0; i<seq0_len; i++) {
//		printf("%d: %c%c\n", i, s0[i], s0r[i]);
//	}

	for (int i=0; i<imid0; i++) {
		c0[i].h = -(i+1)*dna_gap_ext - dna_gap_open*(type_s!=TYPE_GAP_2);
		c0[i].e = -INF;
	}
	for (int i=0; i<imid1; i++) {
		c1[i].h = -(i+1)*dna_gap_ext - dna_gap_open;//*(type_e!=TYPE_GAP_2);
		c1[i].e = -INF;
	}
	r0[0].h = r0[0].f = c0[imid0-1].h;
	r1[0].h = r1[0].f = c1[imid1-1].h;


	crosspoint_t cross;

	int d0 = (type_s!=TYPE_MATCH)?-INF:0;
	int d1 = (type_e!=TYPE_MATCH)?-INF:0;
	for (int j=0; j<seq1_len; j++) {
		int h0 = -(j+1)*dna_gap_ext - dna_gap_open*(type_s!=TYPE_GAP_1);
		cell_t rr0 = processCol(s0, s1[j], d0, h0, c0, imid0);
		d0 = h0;

		int h1 = -(j+1)*dna_gap_ext - dna_gap_open;//*(type_e!=TYPE_GAP_1);
		cell_t rr1 = processCol(s0r, s1r[j], d1, h1, c1, imid1);
		d1 = h1;

		if (j+1<=jmid1) {
			r0[j+1] = rr0;
			r1[j+1] = rr1;
		}
		if (DEBUG) printf("%d: %d %d  (%d %d) (%d %d)\n", j, rr0.h, rr1.h, imid0, imid1, jmid0, jmid1);

		if (j+1>=jmid1) {
			if (match(rr0, r1[seq1_len-(j+1)], diff, &cross)) {
				cross.j = j0+(j+1);
				cross.i = imid0+i0;
				cross.score += score_s;
				return cross;
			}
			if (match(r0[seq1_len-(j+1)], rr1, diff, &cross)) {
				cross.j = j0+(seq1_len-(j+1));
				cross.i = imid0+i0;
				cross.score += score_s;
				return cross;
			}
		}
	}
	if (DEBUG) printf("NOT FOUND %d\n", diff);
	fprintf(stderr, "NOT FOUND %d (%d %d %d %d - %d %d %d %d)\n", diff, type_s, i0, j0, score_s, type_e, i1, j1, score_e);
	exit(1);
}

static crosspoint_t ort_split(Sequence *seq0, Sequence *seq1, int i0, int j0, int i1, int j1, 
						int type_s, int type_e, int score_s, int score_e,
						int *h0, int *h1, int *e0, int *e1) {
	
	if (DEBUG) printf("%d %d %d %d %d %d\n", i0, j0, i1, j1, type_s, type_e, score_s, score_e);
	
	int seq0_len = i1-i0;
	int seq1_len = j1-j0;
	
	if (seq1_len >= H_MAX) {
		fprintf(stderr, "Partition size is too large (%d > %d). You need to store more special rows in stages 1-3.\n", seq1_len, H_MAX);
		exit(1);
	}
	
	int mid = seq0_len/2;
	int mid0 = mid;
	int mid1 = seq0_len - mid;
	
	/* Forward */
	
	const char* s0 = seq0->getData()+(i0-1);
	const char* s1 = seq1->getData()+(j0-1);
	
	for (int j=1; j<=seq1_len; j++) {
		h0[j] = -j*dna_gap_ext - dna_gap_open*(type_s!=TYPE_GAP_1);
		e0[j] = -INF;
	}
	h0[0] = (type_s!=TYPE_MATCH)?-INF:0;
	e0[0] = (type_s!=TYPE_GAP_2)?-INF:0;
	
	for (int i=1; i<=mid0; i++) {
		int h_tmp = h0[0];
		int h_next;
		h_next = h0[0] = e0[0] = -i*dna_gap_ext - dna_gap_open*(type_s!=TYPE_GAP_2);
		int f0 = -INF;
		const char s=s0[i];
		//if (DEBUG) printf("%2d/%2d ", h0[0], e0[0]);
		for (int j=1; j<=seq1_len; j++) {
			e0[j] = MAX(h0[j]-dna_gap_first, e0[j]-dna_gap_ext);
			f0 = MAX(h_next-dna_gap_first, f0-dna_gap_ext);
			h_next = MAX3(h_tmp+((s==s1[j])?dna_match:dna_mismatch), e0[j], f0);
			h_tmp = h0[j];
			h0[j] = h_next;
			//if (DEBUG) printf("%2d/%2d ", h_next, e0[j]);
		}
		//if (DEBUG) printf("\n");
		//printf("FW: %d/%d\n", i, mid0);
	}
	
	
	if (DEBUG) printf("---------\n");
	/* Reverse */
	
	
	
	//int diff = 	(score_e + (type_e == 1 ? dna_gap_open : 0)) - (score_s + (type_s == 1 ? dna_gap_open : 0));
	int diff = 	(score_e + (type_e == TYPE_MATCH ? 0 : dna_gap_open)) - score_s;
	
	
	const char* s0r = seq0->getData()+(i1-1);
	const char* s1r = seq1->getData()+(j1-1);
	
	for (int i=1; i<=mid1; i++) {
		h1[i] = -i*dna_gap_ext - dna_gap_open*(type_e!=TYPE_GAP_2);
		e1[i] = -INF;
	}
	h1[0] = (type_e!=TYPE_MATCH)?-INF:0;
	e1[0] = (type_e!=TYPE_GAP_1)?-INF:0;
	
	{
		int ff = -dna_gap_open*(type_e!=TYPE_GAP_2) - dna_gap_ext*mid1;
		int hh = h1[mid1];
		if (DEBUG) printf("%2d: %2d/%2d\n", (seq1_len-1) + j0, hh, ff);
		
		int* _h0 = &h0[seq1_len];
		int* _e0 = &e0[seq1_len];
		
		int sum_match = _h0[0] + hh;
		int sum_gap = _e0[0] + ff + dna_gap_open;
		
		if (sum_match == diff) {
			crosspoint_t pt;
			pt.i = mid0+i0;
			pt.j = (seq1_len)+j0;
			pt.type = TYPE_MATCH;
			pt.score = score_s + _h0[0];
			return pt;
		} else if (sum_gap == diff) {
			crosspoint_t pt;
			pt.i = mid0+i0;
			pt.j = (seq1_len)+j0;
			pt.type = TYPE_GAP_2;
			pt.score = score_s + _e0[0];
			return pt;
		}
	}
	
	int* _h0 = &h0[seq1_len];
	int* _e0 = &e0[seq1_len];
	
	
	for (int j=1; j<=seq1_len; j++) {
		int h_tmp = h1[0];
		int h_next;
		h_next = h1[0] = e1[0] = -j*dna_gap_ext - dna_gap_open*(type_e!=TYPE_GAP_1);
		int f1 = -INF;
		const char s=s1r[-(j-1)];
		//if (DEBUG) printf("%2d/%2d ", h1[0], e1[0]);
		for (int i=1; i<=mid1; i++) {
			e1[i] = MAX(h1[i]-dna_gap_first, e1[i]-dna_gap_ext);
			f1 = MAX(h_next-dna_gap_first, f1-dna_gap_ext);
			h_next = MAX3(h_tmp+((s==s0r[-(i-1)])?dna_match:dna_mismatch), e1[i], f1);
			h_tmp = h1[i];
			h1[i] = h_next;
			
			//if (DEBUG) printf("%2d/%2d ", h_next, e1[i]);
		}
		//if (DEBUG) printf("\n");
		
		int sum_match = _h0[-j] + h1[mid1];
		int sum_gap = _e0[-j] + f1 + dna_gap_open;
		
		if (sum_match == diff) {
			crosspoint_t pt;
			pt.i = mid0+i0;
			pt.j = (seq1_len-j)+j0;
			pt.type = TYPE_MATCH;
			pt.score = score_s + _h0[-j];
			return pt;
		} else if (sum_gap == diff) {
			crosspoint_t pt;
			pt.i = mid0+i0;
			pt.j = (seq1_len-j)+j0;
			pt.type = TYPE_GAP_2;
			pt.score = score_s + _e0[-j];
			return pt;
		}
			
			if (DEBUG) printf("%2d: %2d/%2d   %2d/%2d    %2d\n", (seq1_len-j-1)+j0, h1[mid1], f1, sum_match, sum_gap, diff);
	}
	
	if (DEBUG) printf("NOT FOUND %d\n", diff);
	fprintf(stderr, "NOT FOUND %d (%d %d %d %d - %d %d %d %d)\n", diff, type_s, i0, j0, score_s, type_e, i1, j1, score_e);
	/*{
		crosspoint_t pt;
		pt.type = -1;
		return pt;
	}*/
	exit(1);
	
	
	
	/*
	s0 = seq0->forward_data+(i1-1);
	s1 = seq1->forward_data+(j1-1);
	
	for (int j=1; j<=seq1_len; j++) {
		h1[j] = -j*dna_gap_ext - dna_gap_open*(type_e!=2);
		e1[j] = -INF;
	}
	h1[0] = (type_e!=0)?-INF:0;
	e1[0] = (type_e!=1)?-INF:0;
	
	for (int i=1; i<=mid1; i++) {
		int h_tmp = h1[0];
		int h_next;
		h_next = h1[0] = e1[0] = -i*dna_gap_ext - dna_gap_open*(type_e!=1);
		int f1 = -INF;
		const char s=s0[-(i-1)];
		if (PRINT) printf("%2d/%2d ", h1[0], e1[0]);
		for (int j=1; j<=seq1_len; j++) {
			e1[j] = MAX(h1[j]-DNA_GAP_FIRST, e1[j]-dna_gap_ext);
			f1 = MAX(h_next-DNA_GAP_FIRST, f1-dna_gap_ext);
			h_next = MAX3(h_tmp+((s==s1[-(j-1)])?dna_match:dna_mismatch), e1[j], f1);
			h_tmp = h1[j];
			h1[j] = h_next;
			
			if (PRINT) printf("%2d/%2d ", h_next, e1[j]);
		}
		if (PRINT) printf("\n");
		//printf("RV: %d/%d\n", i, mid0);
	}
	*/
	
	/* Compare */
	
	
	/*
	s0 = seq0->forward_data+(i0-1);
	s1 = seq1->forward_data+(j0-1);
	
	int ii, jj, tt, best, ss;
	best = -INF;
	//tt = -1;
	ii = mid0;
	char s = s0[ii];
	if (PRINT) printf("mid0: %d (%d)\n", mid0 , ii+i0);
	if (PRINT) printf("mid1: %d (%d)\n", mid1 , ii+i0);
	if (PRINT) printf("len: s0 %d   s1 %d\n", seq0_len, seq1_len);
	
	int* _h0 = h0;
	int* _e0 = e0;
	int* _h1 = &h1[seq1_len-(1-1)];
	int* _e1 = &e1[seq1_len-(1-1)];
	
	for (int j=seq1_len; j>=0; j--) { 
		int sum_match = _h0[j] + _h1[-j];
		int sum_gap = _e0[j] + _e1[-j] + dna_gap_open;
		
		if (true || PRINT) printf("%4d: %c[%c]%c x %c[%c]%c   [%2d/%2d][%2d/%2d] %2d/%2d\n", 
			(j-1)+j0, s0[ii-1],s0[ii],s0[ii+1], s1[j-1],s1[j],s1[j+1],
						  _h0[j], _e0[j],
						  _h1[-j], _e1[-j], 
						  sum_match, sum_gap);
						  
						  if (sum_match > best) {
							  best = sum_match;
							  jj = j;
							  tt = 0;
							  ss = score_s + _h0[j];
						  } 
						  if (sum_gap > best) {
							  best = sum_gap;
							  jj = j;
							  tt = 1;
							  ss = score_s + _e0[j];// + dna_gap_open;
							  						  } 
	}
	if (PRINT) printf("best: %d (score: %d  ss:%d)\n", best, score_s, ss);
	
	printf("DIFF: %d MAX: %d\n", diff, best);
	if (best != diff) fprintf(stderr, "DIFF: %d MAX: %d\n", diff, best);
	
	crosspoint_t pt;
	pt.i = ii+i0;
	pt.j = jj+j0;
	pt.type = tt;
	pt.score = ss;
	if (PRINT) printf("pt: %d,%d,%d,%d\n", pt.i, pt.j, pt.type, pt.score);
	return pt;
	*/
}















/*
*/

static crosspoint_t split(Sequence *seq0, Sequence *seq1, int i0, int j0, int i1, int j1, 
						int type_s, int type_e, int score_s, int score_e,
						int *h0, int *h1, int *e0, int *e1) {

	if (DEBUG) printf("%d %d %d %d %d %d\n", i0, j0, i1, j1, type_s, type_e, score_s, score_e);

    int seq0_len = i1-i0;
    int seq1_len = j1-j0;

    if (seq1_len >= H_MAX) {
        fprintf(stderr, "Partition size is too large (%d > %d). You need to store more special rows in stages 1-3.\n", seq1_len, H_MAX);
        exit(1);
    }

    int mid = seq0_len/2;
    int mid0 = mid;
    int mid1 = seq0_len - mid;


    /* Forward */

    const char* s0 = seq0->getData()+(i0-1);
    const char* s1 = seq1->getData()+(j0-1);

    for (int j=1; j<=seq1_len; j++) {
        h0[j] = -j*dna_gap_ext - dna_gap_open*(type_s!=TYPE_GAP_1);
        e0[j] = -INF;
    }
    h0[0] = (type_s!=TYPE_MATCH)?-INF:0;
    e0[0] = (type_s!=TYPE_GAP_2)?-INF:0;

    for (int i=1; i<=mid0; i++) {
        int h_tmp = h0[0];
        int h_next;
        h_next = h0[0] = e0[0] = -i*dna_gap_ext - dna_gap_open*(type_s!=TYPE_GAP_2);
        int f0 = -INF;
        const char s=s0[i];
        //if (DEBUG) printf("%2d/%2d ", h0[0], e0[0]);
        for (int j=1; j<=seq1_len; j++) {
            e0[j] = MAX(h0[j]-dna_gap_first, e0[j]-dna_gap_ext);
            f0 = MAX(h_next-dna_gap_first, f0-dna_gap_ext);
            h_next = MAX3(h_tmp+((s==s1[j])?dna_match:dna_mismatch), e0[j], f0);
            h_tmp = h0[j];
            h0[j] = h_next;
            //if (DEBUG) printf("%2d/%2d ", h_next, e0[j]);
        }
        //if (DEBUG) printf("\n");
        //printf("FW: %d/%d\n", i, mid0);
    }


    if (DEBUG) printf("---------\n");
    /* Reverse */


    s0 = seq0->getData()+(i1-1);
    s1 = seq1->getData()+(j1-1);

    for (int j=1; j<=seq1_len; j++) {
        h1[j] = -j*dna_gap_ext - dna_gap_open*(type_e!=TYPE_GAP_1);
        e1[j] = -INF;
    }
    h1[0] = (type_e!=TYPE_MATCH)?-INF:0;
    e1[0] = (type_e!=TYPE_GAP_2)?-INF:0;

    for (int i=1; i<=mid1; i++) {
        int h_tmp = h1[0];
        int h_next;
        h_next = h1[0] = e1[0] = -i*dna_gap_ext - dna_gap_open*(type_e!=TYPE_GAP_2);
        int f1 = -INF;
        const char s=s0[-(i-1)];
        //if (DEBUG) printf("%2d/%2d ", h1[0], e1[0]);
        for (int j=1; j<=seq1_len; j++) {
            e1[j] = MAX(h1[j]-dna_gap_first, e1[j]-dna_gap_ext);
            f1 = MAX(h_next-dna_gap_first, f1-dna_gap_ext);
            h_next = MAX3(h_tmp+((s==s1[-(j-1)])?dna_match:dna_mismatch), e1[j], f1);
            h_tmp = h1[j];
            h1[j] = h_next;

           // if (DEBUG) printf("%2d/%2d ", h_next, e1[j]);
        }
        //if (DEBUG) printf("\n");
        //printf("RV: %d/%d\n", i, mid0);
    }


    /* Compare */



    s0 = seq0->getData()+(i0-1);
    s1 = seq1->getData()+(j0-1);

    int ii, jj, tt, best, ss;
    best = -INF;
    //tt = -1;
    ii = mid0;
    //char s = s0[ii];
    if (DEBUG) printf("mid0: %d (%d)\n", mid0 , ii+i0);
    if (DEBUG) printf("mid1: %d (%d)\n", mid1 , ii+i0);
    if (DEBUG) printf("len: s0 %d   s1 %d\n", seq0_len, seq1_len);

    int* _h0 = h0;
    int* _e0 = e0;
    int* _h1 = &h1[seq1_len-(1-1)];
    int* _e1 = &e1[seq1_len-(1-1)];

	for (int j=seq1_len; j>=0; j--) { 
        int sum_match = _h0[j] + _h1[-j];
        int sum_gap = _e0[j] + _e1[-j] + dna_gap_open;

        if (DEBUG) printf("%4d: %c[%c]%c x %c[%c]%c   [%2d/%2d][%2d/%2d] %2d/%2d\n",
                (j-1)+j0, s0[ii-1],s0[ii],s0[ii+1], s1[j-1],s1[j],s1[j+1],
                _h0[j], _e0[j],
                _h1[-j], _e1[-j], 
                sum_match, sum_gap);

        if (sum_match > best) {
            best = sum_match;
            jj = j;
            tt = TYPE_MATCH;
            ss = score_s + _h0[j];
        } 
        if (sum_gap > best) {
            best = sum_gap;
            jj = j;
            tt = TYPE_GAP_2;
            ss = score_s + _e0[j];// + dna_gap_open;
            /*if (j==0) {
                printf("FULL GAP\n");
            }*/
        } 
    }
    if (DEBUG) printf("best: %d (score: %d  ss:%d)\n", best, score_s, ss);
    crosspoint_t pt;
    pt.i = ii+i0;
    pt.j = jj+j0;
    pt.type = tt;
    pt.score = ss;
    if (DEBUG) printf("pt: %d,%d,%d,%d\n", pt.i, pt.j, pt.type, pt.score);
    return pt;
}

int merge_partitions(CrosspointsFile* crosspoints, crosspoint_t *new_positions) {
    vector<crosspoint_t> merged_partitions;
    merged_partitions.clear();
    merged_partitions.push_back(crosspoints->at(0));
    int has_new_pos = 0;
    for (int i=1; i<crosspoints->size(); i++) {
        bool diff_pos = (new_positions[i].i!=crosspoints->at(i-1).i || new_positions[i].j!=crosspoints->at(i-1).j);
        if (new_positions[i].type != -1 && diff_pos) {
            has_new_pos = 1;
            merged_partitions.push_back(new_positions[i]);
        }
        merged_partitions.push_back(crosspoints->at(i));
    }
    if (has_new_pos) {
    	crosspoints->clear();
    	crosspoints->assign(merged_partitions.begin(), merged_partitions.end());
    	//crosspoints->save();
    }
    return has_new_pos;
}

static int reduce_partitions(Job* job, CrosspointsFile* crosspoints) {
    /*int i0, j0, i1, j1, start_type, prev_type, prev_score;

    int partition_id = 0;
    i1 = job->crosspoints[partition_id].i;
    j1 = job->crosspoints[partition_id].j;
    prev_type = job->crosspoints[partition_id].type;
    prev_score = job->crosspoints[partition_id].score;*/

    static pthread_t thread[NUM_THREADS];
    static split_args_t args[NUM_THREADS];
    crosspoint_t *new_partitions = (crosspoint_t *)malloc(crosspoints->size()*sizeof(crosspoint_t));

    int num_threads = NUM_THREADS;
    if (num_threads >= crosspoints->size()-1) {
    	num_threads = crosspoints->size()-1;
    }

    for (int i=0; i<num_threads; i++) {
        args[i].job = job;
        args[i].crosspoints = crosspoints;
        args[i].out_pos = new_partitions;
        args[i].partition1 = (crosspoints->size()-1)*(i+1)/num_threads;
        if (i==0) {
            args[0].partition0 = 0;
        } else {
            args[i].partition0 = args[i-1].partition1;
        }
        printf("Thread %d [%d..%d] (%d)\n", i, args[i].partition0, args[i].partition1, crosspoints->size());
    }
    //args[num_threads-1].partition1 = crosspoints->size()-1;

    for (int i=0; i<num_threads; i++) {
        //printf("%d: %d-%d  (%d)\n", i, args[i].partition0, args[i].partition1, job->partitions_count);
        create_split_thread(&args[i], &thread[i]);
    }
    for (int i=0; i<num_threads; i++) {
        int rc = pthread_join(thread[i], NULL);
        if (rc) {
            printf("ERROR; return code from pthread_join() is %d\n", rc);
            exit(-1);
        }
    }
    int has_new_partitions = merge_partitions(crosspoints, new_partitions);
    free(new_partitions);
    return has_new_partitions;
}

int stage4_pool_wait(Job* job, int id) {
	Sequence* seq1 = job->getAlignmentParams()->getSequence(1);

	CrosspointsFile* stage4Crosspoints = new CrosspointsFile(job->getCrosspointFile(STAGE_4, id));
	stage4Crosspoints->loadCrosspoints();

	int j0 = seq1->getTrimStart()-1;
	int j1 = seq1->getTrimEnd();

	if (!job->getAlignerPool()->isLastNode() && stage4Crosspoints->back().j >= j1) {
		CrosspointsFile* tmp = job->getAlignerPool()->receiveCrosspointFile();
		stage4Crosspoints->pop_back();
		stage4Crosspoints->insert(stage4Crosspoints->end(), tmp->begin(), tmp->end());
		delete tmp;
	}

	if (!job->getAlignerPool()->isFirstNode() && stage4Crosspoints->front().j <= j0) {
		job->getAlignerPool()->dispatchCrosspointFile(stage4Crosspoints);
		//exit(1);
		delete stage4Crosspoints;
		return -1;
	} else {
		stage4Crosspoints->writeToFile(job->getCrosspointFile(STAGE_3, id+1));
		delete stage4Crosspoints;
		return 0;
	}

}

void stage4(Job* job, int id) {
	FILE* stats = job->fopenStatistics(STAGE_4, id);
	Sequence* seq0 = job->getAlignmentParams()->getSequence(0);
	Sequence* seq1 = job->getAlignmentParams()->getSequence(1);

	//int seq0_len = seq0->getLen();
	//int seq1_len = seq1->getLen();

	job->getAlignmentParams()->printParams(stats);
	fflush(stats);
	dna_gap_open = -job->getAlignmentParams()->getGapOpen();
	dna_gap_ext  = -job->getAlignmentParams()->getGapExtension();
	dna_match    = job->getAlignmentParams()->getMatch();
	dna_mismatch = job->getAlignmentParams()->getMismatch();
	dna_gap_first = dna_gap_ext + dna_gap_open;
	
	fprintf(stats, "MAXIMUM PARTITION SIZE: %d\n", job->stage4_maximum_partition_size);
	fprintf(stats, "ORTHOGONAL EXECUTION: %s\n", job->stage4_orthogonal_execution?"YES":"NO");
	
	Timer timer2;
	
	int ev_step = timer2.createEvent("STEP");
	int ev_start = timer2.createEvent("START");
	int ev_end = timer2.createEvent("END");
	int ev_crosspoints = timer2.createEvent("CROSSPOINTS");
	int ev_write = timer2.createEvent("WRITE");
	
	timer2.eventRecord(ev_start);
	
	CrosspointsFile* stage3Crosspoints = new CrosspointsFile(job->getCrosspointFile(STAGE_3, id));
	stage3Crosspoints->loadCrosspoints();
	//stage3Crosspoints->reverse(seq0_len, seq1_len);

	CrosspointsFile* crosspoints = new CrosspointsFile(job->getCrosspointFile(STAGE_4, id));
	crosspoints->assign(stage3Crosspoints->begin(), stage3Crosspoints->end());
	delete stage3Crosspoints;
	
	timer2.eventRecord(ev_crosspoints);
	
	int must_write_partitions = 0;
	int step = 1;
	int max_i, max_j;
	float step_sum = 0;
	while (crosspoints->getLargestPartitionSize(&max_i, &max_j) > job->stage4_maximum_partition_size) {
		int crosspoints_count = crosspoints->size();
		if (step == 1) {
			fprintf(stats, "-step %2d  max size: %5dx%5d crosspoints: %8d   time: %.4f   sum:%.4f\n", 
					0, max_i, max_j, crosspoints_count, 0.0f, 0.0f);
			fflush(stats);
		}
		if (!reduce_partitions(job, crosspoints)) {
			fprintf(stderr, "Didn't reduce partition.\n");
            // TODO tratar erro? não houve redução!
            break;
        }
		float step_diff = timer2.eventRecord(ev_step);
		step_sum += step_diff;
		fprintf(stats, " step %2d  max size: %5dx%5d crosspoints: %8d   time: %.4f   sum:%.4f\n", 
				step, max_i, max_j, crosspoints_count, step_diff, step_sum);
		fflush(stats);
		//timer2.eventRecord(ev_write);
		step++;
	}
	float step_diff = timer2.eventRecord(ev_step);
	step_sum += step_diff;
	fprintf(stats, "-step %2d  max size: %5dx%5d crosspoints: %8d   time: %.4f   sum:%.4f\n", 
			step, max_i, max_j, crosspoints->size(), step_diff, step_sum);
	fflush(stats);
	timer2.eventRecord(ev_start);
	
    crosspoints->save();
	delete crosspoints;

	timer2.eventRecord(ev_write);
	
    if (DEBUG) printf("\n");
	timer2.eventRecord(ev_end);
	
	fprintf(stats, "Stage4 times:\n");
	float diff = timer2.printStatistics(stats);

	
	fprintf(stats, "        total: %.4f\n", diff);
	fclose(stats);
}

