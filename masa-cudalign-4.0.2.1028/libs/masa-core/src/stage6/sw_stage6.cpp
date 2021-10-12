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

#include "sw_stage6.h"

static int dna_gap_open;
static int dna_gap_ext;
static int dna_match;
static int dna_mismatch;

#define DEBUG (0)

//#define USE_CAIRO

static void printText(Alignment* alignment, string filename);
// TODO somente texto
#ifdef USE_CAIRO   
static void drawAlignment(AligAlignment* alignment, string filename);
static void drawHistogram(AligAlignment* alignment, string filename);
#endif

output_format_t stage6_formats[] = {
    {"Text", "Textual representation.", printText},
#ifdef USE_CAIRO    
    {"Plot", "Graphical representation in SVG.", drawAlignment},
    {"Hist", "Histogram representation in SVG.", drawHistogram},
#endif
    {NULL, NULL, NULL}
};


static void printText(Alignment* alignment, string filename) {
	printf("PRINT TEXT\n"); fflush(stdout);
	Sequence* seq0 = new Sequence(alignment->getAlignmentParams()->getSequence(0));
	Sequence* seq1 = new Sequence(alignment->getAlignmentParams()->getSequence(1));
	
	const char* seq0_data = seq0->getForwardData();
	const char* seq1_data = seq1->getForwardData();

	FILE* file = fopen(filename.c_str(), "w");

	fprintf(file, "Query: %s ", seq0->getInfo()->getDescription().c_str());
	if (seq0->getInfo()->getSize() == seq0->getLen()) {
		fprintf(file, "(%d)\n", seq0->getLen());
	} else {
		fprintf(file, "[%d..%d](%d)\n", seq0->getTrimStart(), seq0->getTrimEnd(), seq0->getLen());
	}
	fprintf(file, "Sbjct: %s ", seq1->getInfo()->getDescription().c_str());
	if (seq0->getInfo()->getSize() == seq1->getLen()) {
		fprintf(file, "(%d)\n", seq1->getLen());
	} else {
		fprintf(file, "[%d..%d](%d)\n", seq1->getTrimStart(), seq1->getTrimEnd(), seq1->getLen());
	}
    fprintf(file, "\n");

    /*seq0->restore();
    seq1->restore();

	seq0->loadFile();
	seq1->loadFile();*/
	
	AlignmentParams* alignmentParams = alignment->getAlignmentParams();
	int i0 = alignment->getStart(0);
	int j0 = alignment->getStart(1);
	int i1 = alignment->getEnd(0);
	int j1 = alignment->getEnd(1);
	// i0,j0,i1,j1 are 1-based

	vector<gap_t> gaps0 = *alignment->getGaps(0);
	vector<gap_t> gaps1 = *alignment->getGaps(1);

	printf("(i0,j0) = (%d,%d)\n", i0, j0);
	printf("(i1,j1) = (%d,%d)\n", i1, j1);

	printf("FILE: %p %s\n", file, filename.c_str());

	char query[100];
	char subject[100];

	int score = 0;
	int gap_openings = 0;
	int gap_extentions = 0;
	int matches = 0;
	int mismatches = 0;

	int qgap = 0;
	int sgap = 0;
	int COLS = 60;


	int dir_i = (i1 > i0) ? +1 : -1;
	int dir_j = (j1 > j0) ? +1 : -1;

	int c0 = (dir_i > 0) ? 0 : gaps0.size()-1;
	int c1 = (dir_j > 0) ? 0 : gaps1.size()-1;

	const gap_t end_gap(-1, -1);

	gap_t current_gap0 = (c0<0 || c0>=gaps0.size()) ? end_gap : gaps0[c0];
	gap_t current_gap1 = (c1<0 || c1>=gaps1.size()) ? end_gap : gaps1[c1];

	int i = i0;
	int j = j0;
	int end_i = false;//i == i1;
	int end_j = false;//j == j1; //TODO testar caso de alinhamento nulo

	if (i0 == -1 && j0 == -1 && i1 == -1 && j1 == -1) {
		// Empty Alignment
		end_i = true;
		end_j = true;

		fprintf(file, "There was no alignment produced!\n\n");
	}

	while (!end_i || !end_j) {
		if (DEBUG) printf("[%3d]%5d.%3d  [%3d]%5d.%3d\n", c0, current_gap0.pos, current_gap0.len, c1, current_gap1.pos, current_gap1.len);
		int qn = 0;
		int qp = i;
		for (int k=0; k<COLS && !end_i; k++) {
			if (current_gap0.pos == i+(dir_i>0?0:1)) {
				query[qn++] = '-';
				current_gap0.len--;
				if (current_gap0.len==0) {
					c0 += dir_i;
					current_gap0 = (c0<0 || c0>=gaps0.size()) ? end_gap : gaps0[c0];
				}
			} else {
				query[qn++] = seq0_data[i-1];
				if (i == i1) {
					end_i = true;
					break;
				}
				i += dir_i;
			}
		}

		int sn = 0;
		int sp = j;
		for (int k=0; k<COLS && !end_j; k++) {
			if (current_gap1.pos == j+(dir_j>0?0:1)) {
				subject[sn++] = '-';
				current_gap1.len--;
				if (current_gap1.len==0) {
					c1 += dir_j;
					current_gap1 = (c1<0 || c1>=gaps1.size()) ? end_gap : gaps1[c1];
				}
			} else {
				subject[sn++] = seq1_data[j-1];
				if (j == j1) {
					end_j = true;
					break;
				}
				j += dir_j;
			}
		}

		if (sn < qn) {
			for (int i=sn; i<qn; i++) {
				subject[i] = '-';
			}
			sn=qn;
		} else {
			for (int i=qn; i<sn; i++) {
				query[i] = '-';
			}
			qn=sn;
		}
		query[qn] = '\0';
		subject[sn] = '\0';

		fprintf(file, "Query: %8d %s %8d\n", qp, query, i/*+(query[qn-1]=='-')*/);
		fprintf(file, "                ");
		int temp = 0;
		for (int k=0; k<sn || k<qn; k++) {
			fprintf(file, "%c", (query[k]==subject[k])?'|':' ');
			if (query[k] == '-') {
				if (qgap) {
					temp += dna_gap_ext;
					gap_extentions++;
				} else {
					temp += dna_gap_open+dna_gap_ext;
					gap_openings++;
					gap_extentions++;
				}
				qgap = 1;
				sgap = 0;
			} else if (subject[k] == '-') {
				if (sgap) {
					temp += dna_gap_ext;
					gap_extentions++;
				} else {
					temp += dna_gap_open+dna_gap_ext;
					gap_openings++;
					gap_extentions++;
				}
				qgap = 0;
				sgap = 1;
			} else {
				if (query[k]==subject[k]) {
					temp += dna_match;
					matches++;
				} else {
					temp += dna_mismatch;
					mismatches++;
				}
				//temp += ((query[k]==subject[k])?(DNA_MATCH):(DNA_MISMATCH));
				qgap = 0;
				sgap = 0;
			}
		}
		score += temp;
		fprintf(file, " [%d/%d]\n", temp,score);
		//fprintf(file, "\n");

		fprintf(file, "Sbjct: %8d %s %8d\n", sp, subject, j /*+ (subject[sn-1]=='-')*/);
		fprintf(file, "\n\n");
	}
	if (score != alignment->getRawScore()) {
		fprintf(stderr, "Stage6 error: Alignment score is different (%d != %d)\n", score, alignment->getRawScore());
		exit(1);
	}

	fprintf(file, "Summary:\n\n");
	fprintf(file, "Total Score:    %10d\n", score);
	fprintf(file, "Matches:        %10d (+%d)\n", matches, dna_match);
	fprintf(file, "Mismatches:     %10d (%d)\n", mismatches, dna_mismatch);
	fprintf(file, "Gap Openings:   %10d (%d)\n", gap_openings, dna_gap_open);
	fprintf(file, "Gap Extentions: %10d (%d)\n", gap_extentions, dna_gap_ext);

	delete seq0;
	delete seq1;
}


#ifdef USE_CAIRO
#include <cairo.h>
#include <cairo-svg.h>
#include <math.h>
#include <algorithm>

#define IMAGE_SIZE 		1000.0f
#define MIN_TICK_STEP	5.0f
#define MARGIN_LEFT		50.0f
#define MARGIN_TOP		50.0f
#define MARGIN_RIGHT	100.0f
#define MARGIN_BOTTOM	50.0f
#define TEXT_HEIGHT 	100.0f

struct my_rect_t {
	double x0, y0, x1, y1;
	double i0, j0, i1, j1;
	double width() {
		return x1-x0;
	}
	double height() {
		return y1-y0;
	}
	double toY(double i) {
		return ((i-i0)/(i1-i0))*(y1-y0)+y0;
	}
	double toX(double j) {
		return ((j-j0)/(j1-j0))*(x1-x0)+x0;
	}
	void incX(double x) {
		x0 += x;
		x1 += x;
	}
	void incY(double y) {
		y0 += y;
		y1 += y;
	}
	void validate() {
		if (x0<0) {
			incX(-x0);
		}
		if (y0<0) {
			incY(-y0);
		}
	}
};


bool sort_gaps_f (gap_sequence_t g1, gap_sequence_t g2) { 
	return (g1.len()>g2.len()); 
}

bool sort_gaps_pos_f (gap_sequence_t g1, gap_sequence_t g2) { 
	return g1.j1 > g2.j1;
}


static void drawPruningArea(Job* job, cairo_t* cr, my_rect_t* rect) {
	Sequence* seq0 = &job->seq[0];
	Sequence* seq1 = &job->seq[1];
	
	const int seq0_len = seq0->getLen();
	const int seq1_len = seq1->getLen();
	
	FILE* dump = fopen(job->dump_pruning_text_filename.c_str(), "rt");
	fprintf(stderr, "dump %s %p\n", job->dump_pruning_text_filename.c_str(), dump);
	if (dump == NULL) return;
	
	cairo_rectangle(cr, rect->x0, rect->y0, rect->width(), rect->height());
	cairo_clip(cr);
	
	/*cairo_move_to(cr,0,0);
	cairo_line_to(cr,50,50);
	cairo_line_to(cr,40,60);
	cairo_stroke(cr);*/
	
	vector<int> k_d;
	vector<int> k_s;
	vector<int> k_e;
	int d, s, e;
	while (fscanf(dump, "%d%d%d\n", &d, &s, &e) != EOF) {
		k_d.push_back(d);
		k_e.push_back(e);
		k_s.push_back(s);
	}
	int blocks = k_e[0];
	int max_d = k_d[k_d.size()-1]-blocks;
	
	cairo_set_line_width (cr, 0.1);
	cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
	
	cairo_move_to(cr, rect->toX(0), rect->toY(0));
	for (int i=1; i<k_d.size();i++) {
		float i0 = ((float)k_d[i]-k_s[i])*seq0_len/max_d;
		float j0 = ((float)k_s[i])*seq1_len/blocks;
		cairo_line_to(cr, rect->toX(j0), rect->toY(i0));
	}
	cairo_line_to(cr, rect->toX(seq1_len), rect->toY(seq0_len));
	cairo_line_to(cr, rect->toX(0), rect->toY(seq0_len));
	cairo_fill(cr);

	
	cairo_move_to(cr, rect->toX(seq1_len), rect->toY(0));
	for (int i=1; i<k_d.size();i++) {
		float i0 = ((float)k_d[i]-k_e[i])*seq0_len/max_d;
		float j0 = ((float)k_e[i])*seq1_len/blocks;
		cairo_line_to(cr, rect->toX(j0), rect->toY(i0));
	}
	cairo_line_to(cr, rect->toX(seq1_len), rect->toY(seq0_len));
	cairo_fill(cr);
	fclose(dump);
	
	cairo_reset_clip(cr);
}


static void drawBackground(Job* job, cairo_t* cr, my_rect_t* rect) {
	cairo_set_line_width (cr, 0.1);
	
	cairo_rectangle(cr, rect->x0, rect->y0, rect->width(), rect->height());
	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	cairo_fill(cr);
	
	cairo_rectangle(cr, rect->x0, rect->y0, rect->width(), rect->height());
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_set_line_width(cr, 1);
	cairo_stroke(cr);
}

static void drawGaps(Job* job, cairo_t* cr, my_rect_t* rect,
					 vector<gap_sequence_t>* gaps) {
	cairo_rectangle(cr, rect->x0, rect->y0, rect->width(), rect->height());
	cairo_clip(cr);
	
	cairo_move_to (cr, rect->toX(gaps->at(0).j0), rect->toY(gaps->at(0).i0));
	//cairo_move_to (cr, rect->x0, rect->y0);
	for (int k=0; k<gaps->size(); k++) {
		gap_sequence_t gap = gaps->at(k);
		if (gap.i0 < rect->i0 || gap.j0 < rect->j0) continue;
		if (gap.i1 > rect->i1 || gap.j1 > rect->j1) break;
		cairo_line_to (cr, rect->toX(gap.j0), rect->toY(gap.i0));
		cairo_line_to (cr, rect->toX(gap.j1), rect->toY(gap.i1));
		//printf("DG (%d,%d)-(%d,%d) %lf\n", gap.i0, gap.j0, gap.i1, gap.j1, rect->toX(gap.j0));
	}
	
	//cairo_line_to (cr, rect->x1, rect->y1);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND); 
	cairo_set_line_width(cr, 2.0f);
	cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
	cairo_stroke(cr);
	
	cairo_reset_clip(cr);
}


static void drawLargestGaps(Job* job, cairo_t* cr, my_rect_t* rect, int count) {
	Sequence* seq0 = &job->seq[0];
	Sequence* seq1 = &job->seq[1];
	
	const int seq0_len = seq0->getLen();
	const int seq1_len = seq1->getLen();
	
	vector<gap_sequence_t>* gaps_in = job->alignment->getGapSequences();
	vector<gap_sequence_t> highlight;
	
	int max_dist = ((rect->i1-rect->i0)+(rect->j1-rect->j0))/100;
	
	for (int k=0; k<gaps_in->size();) {
		gap_sequence_t gap0 = gaps_in->at(k);
		gap_sequence_t h;
		h.i0 = gap0.i0;
		h.j0 = gap0.j0;
		h.i1 = gap0.i1;
		h.j1 = gap0.j1;
		int next_k = k+1;
		for (int l=k+1; l<gaps_in->size(); l++) {
			gap_sequence_t gap1 = gaps_in->at(l);
			if ( (gap1.i0-h.i1) + (gap1.j0-h.j1) > max_dist) break;
			int diff_i = gap1.i1 - h.i0;
			int diff_j = gap1.j1 - h.j0;
			float ratio = ((float)diff_i)/diff_j;
			const float MAX_RATIO = 0.75f;
			if (ratio < MAX_RATIO || ratio > 1/MAX_RATIO) {
				h.i1 = gap1.i1;
				h.j1 = gap1.j1;
				next_k = l+1;
			}
		}
		highlight.push_back(h);
		k = next_k;
	}
	sort (highlight.begin(), highlight.end(), sort_gaps_f);
	vector<gap_sequence_t> highlight_ordered;
	
	for (int k=0; k<count && k<highlight.size(); k++) {
		highlight_ordered.push_back(highlight[k]);
	}
	sort (highlight_ordered.begin(), highlight_ordered.end(), sort_gaps_pos_f);
	
	
	vector<my_rect_t> zoom_rects;
	for (int k=0; k<highlight_ordered.size(); k++) {
		gap_sequence_t gap = highlight_ordered[k];
		printf("%d: (%d,%d)-(%d,%d) = %d\n", k, gap.i0, gap.j0, gap.i1, gap.j1, gap.len());
		//cairo_arc(cr, toPoint(gap.j0, seq1_len, size_x), toPoint(gap.i0, seq0_len, size_y), CIRCLE_SIZE, 0.0, 2 * M_PI);
		
		float xx0 = rect->toX(gap.j0);
		float xx1 = rect->toX(gap.j1);
		float yy0 = rect->toY(gap.i0);
		float yy1 = rect->toY(gap.i1);
		
		float plus = 100;
		float mult;
		
		int dir = 1;
		if (k%2==0) dir=-1;
		
		my_rect_t zoom_rect;
		
		zoom_rect.x0 = xx0+plus*dir;
		zoom_rect.x1 = xx1+plus*dir;
		zoom_rect.y0 = yy0-plus*dir;
		zoom_rect.y1 = yy1-plus*dir;
		
		if (xx1-xx0 > yy1-yy0) {
			mult = (IMAGE_SIZE/15)/(xx1-xx0);
		} else {
			mult = (IMAGE_SIZE/15)/(yy1-yy0);
		}
		
		zoom_rect.x0 -= (xx1-xx0)*mult;
		zoom_rect.x1 += (xx1-xx0)*mult;
		zoom_rect.y0 -= (yy1-yy0)*mult;
		zoom_rect.y1 += (yy1-yy0)*mult;
		
		const float MARGIN = 0.10f;
		int margin_i = (gap.i1 - gap.i0)*MARGIN;
		int margin_j = (gap.j1 - gap.j0)*MARGIN;
		zoom_rect.i0 = gap.i0 - margin_i;
		zoom_rect.j0 = gap.j0 - margin_j;
		zoom_rect.i1 = gap.i1 + margin_i;
		zoom_rect.j1 = gap.j1 + margin_j;
		
		
		for (int k=0; k<zoom_rects.size(); k++) {
			my_rect_t r = zoom_rects[k];
			if ((zoom_rect.x1 < r.x1 && zoom_rect.x1 > r.x0) && (zoom_rect.y1 < r.y1 && zoom_rect.y1 > r.y0)) {
				float diffX = (zoom_rect.x1-r.x0);
				float diffY = (zoom_rect.y1-r.y0);
				zoom_rect.incY(-diffY*1.1);
				zoom_rect.incX(-diffX*1.1);
			}
		}
		
		zoom_rect.validate();
		zoom_rects.push_back(zoom_rect);
	}
	
	
	for (int k=0; k<zoom_rects.size(); k++) {
		my_rect_t zoom_rect = zoom_rects[k];
		
		float xx0 = rect->toX(zoom_rect.j0);
		float xx1 = rect->toX(zoom_rect.j1);
		float yy0 = rect->toY(zoom_rect.i0);
		float yy1 = rect->toY(zoom_rect.i1);
		
		cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
		static const double dashed3[] = {4.0};
		cairo_set_dash(cr, dashed3, 1, 0);
		cairo_move_to (cr, xx0, yy0);
		cairo_line_to (cr, zoom_rect.x0, zoom_rect.y0);
		cairo_stroke(cr);
		cairo_move_to (cr, xx1, yy1);
		cairo_line_to (cr, zoom_rect.x1, zoom_rect.y1);
		cairo_stroke(cr);
		cairo_set_dash(cr, NULL, 0, 0);
		
		drawGaps(job, cr, &zoom_rect, gaps_in);
		
		cairo_set_source_rgb(cr, 0.8, 0.2, 0.1);
		cairo_set_line_width(cr, 2);
		cairo_rectangle(cr, xx0, yy0, xx1-xx0, yy1-yy0);
		cairo_stroke(cr);
		
	}
	
}
	
	
static void draw_accession_numbers(Job* job, cairo_t* cr, my_rect_t* rect) {
	char text[500];
	cairo_text_extents_t te;
	
	cairo_set_font_size(cr, TEXT_HEIGHT/2);
	sprintf(text, "%s", job->seq1->accession.c_str());
	cairo_text_extents (cr, text, &te);
	cairo_move_to(cr, rect->x0+(rect->width()-te.width)/2, rect->y1+TEXT_HEIGHT/2);
	cairo_show_text(cr, text);
	
	cairo_set_font_size(cr, TEXT_HEIGHT/2);
	sprintf(text, "%s", job->seq0->accession.c_str());
	cairo_text_extents (cr, text, &te);
	cairo_move_to(cr, rect->x1+TEXT_HEIGHT/6, rect->y0+(rect->height()-te.width)/2);
	cairo_save (cr);
	cairo_rotate (cr, M_PI / 2.0);
	cairo_show_text(cr, text);
	cairo_restore (cr);
}	
	
static void draw_ticks(Job* job, cairo_t* cr, my_rect_t* rect) {
	int diff_i = rect->i1-rect->i0;
	int diff_j = rect->j1-rect->j0;
	
	int step_ticks_i = diff_i/3;
	int step_ticks_j = diff_j/3;
	step_ticks_i = pow(10, (int)(log10(step_ticks_i)));
	step_ticks_j = pow(10, (int)(log10(step_ticks_j)));
	if ((rect->toX(step_ticks_j)-rect->toX(0))/10 < MIN_TICK_STEP) {
		step_ticks_j *= 10;
	}
	if ((rect->toY(step_ticks_i)-rect->toY(0))/10 < MIN_TICK_STEP) {
		step_ticks_i *= 10;
	}
	int tick_i0 = (rect->i0/(step_ticks_i/10)+1)*(step_ticks_i/10);
	int tick_j0 = (rect->j0/(step_ticks_j/10)+1)*(step_ticks_j/10);
	
	for (int i=tick_i0; i<=rect->i1; i+=step_ticks_i/10) {
		int len = (i%(step_ticks_i))==0?TEXT_HEIGHT/3:TEXT_HEIGHT/8;
		cairo_move_to(cr, rect->x1-len, rect->toY(i));
		cairo_line_to(cr, rect->x1, rect->toY(i));
		cairo_stroke(cr);
		cairo_move_to(cr, rect->x0, rect->toY(i));
		cairo_line_to(cr, rect->x0+len, rect->toY(i));
		cairo_stroke(cr);
	}
	for (int j=tick_j0; j<=rect->j1; j+=step_ticks_j/10) {
		int len = (j%(step_ticks_j))==0?TEXT_HEIGHT/3:TEXT_HEIGHT/8;
		cairo_move_to(cr, rect->toX(j), rect->y1);
		cairo_line_to(cr, rect->toX(j), rect->y1-len);
		cairo_stroke(cr);
		cairo_move_to(cr, rect->toX(j), rect->y0);
		cairo_line_to(cr, rect->toX(j), rect->y0+len);
		cairo_stroke(cr);
	}
	
	char text[50];
	cairo_text_extents_t te;

	cairo_set_font_size(cr, TEXT_HEIGHT/3);
	sprintf(text, "%.0f", rect->j0);
	cairo_move_to(cr, rect->x0, rect->y0-TEXT_HEIGHT/12);
	cairo_show_text(cr, text);
	sprintf(text, "%.0f", rect->j1);
	cairo_text_extents (cr, text, &te);
	cairo_move_to(cr, rect->x1-te.width, rect->y0-TEXT_HEIGHT/12);
	cairo_show_text(cr, text);
	
	
	cairo_set_font_size(cr, TEXT_HEIGHT/3);
	sprintf(text, "%.0f", rect->i0);
	cairo_move_to(cr, rect->x0-TEXT_HEIGHT/3, rect->y0);
	cairo_save (cr);
	cairo_rotate (cr, M_PI / 2.0);
	//cairo_move_to(cr, 0, rect->height()+TEXT_HEIGHT/3);
	cairo_show_text(cr, text);
	cairo_restore(cr);
	
	sprintf(text, "%.0f", rect->i1);
	cairo_text_extents (cr, text, &te);
	cairo_move_to(cr, rect->x0-TEXT_HEIGHT/3, rect->y1-te.width);
	cairo_save (cr);
	cairo_rotate (cr, M_PI / 2.0);
	cairo_show_text(cr, text);
	cairo_restore(cr);
	
}



static void drawAlignment(Job* job) {
	cairo_surface_t *surface;
	cairo_t *cr;
	
	Sequence* seq0 = &job->seq[0];
	Sequence* seq1 = &job->seq[1];

	const int seq0_len = seq0->getLen();
	const int seq1_len = seq1->getLen();
	
	int i0=job->alignment->i0;
	int j0=job->alignment->j0;
	int i1=job->alignment->i1;
	int j1=job->alignment->j1;
	
	if (job->dump_pruning) {
		i0 = 0;
		j0 = 0;
		i1 = seq0_len;
		j1 = seq1_len;
	}
	int delta_i = i1-i0;
	int delta_j = j1-j0;
	
	vector<gap_sequence_t>* gaps = job->alignment->getGapSequences();
	
	int cgaps = gaps->size();
	
	
	float size_x;
	float size_y;
	if (delta_i > delta_j) {
		size_y = IMAGE_SIZE;
		size_x = IMAGE_SIZE*delta_j/delta_i;
		fprintf(stderr, "DELTA_I: %f %f\n", size_x+MARGIN_LEFT+MARGIN_RIGHT, size_y+MARGIN_TOP+MARGIN_BOTTOM);
	} else {
		size_y = IMAGE_SIZE*delta_i/delta_j;
		size_x = IMAGE_SIZE;
	}
	/*const int TEXTY = 100;*/
	
	/* Creating the surface */
	//surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, SIZEX, SIZEY+TEXTY);
	surface = cairo_svg_surface_create(job->alignment_svg_filename.c_str(), size_x+MARGIN_LEFT+MARGIN_RIGHT, size_y+MARGIN_TOP+MARGIN_BOTTOM);
	/* Creating the context */
	cr = cairo_create (surface);
	
	cairo_set_line_width (cr, 0.1);
	
	cairo_rectangle(cr, 0.0, 0.0, size_x+MARGIN_LEFT+MARGIN_RIGHT, size_y+MARGIN_TOP+MARGIN_BOTTOM);
	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	cairo_fill(cr);

	/*cairo_rectangle(cr, 0.0, 0.0, size_x, size_y);
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_set_line_width(cr, 1);
	cairo_stroke(cr);*/
	
	/*cairo_move_to (cr, toPoint(j0, seq1_len, size_x), toPoint(i0, seq0_len, size_y));
	for (int k=0; k<gaps.size(); k++) {
		gap_sequence_t gap = gaps[k];
		cairo_line_to (cr, toPoint(gap.j0, seq1_len, size_x), toPoint(gap.i0, seq0_len, size_y));
		cairo_line_to (cr, toPoint(gap.j1, seq1_len, size_x), toPoint(gap.i1, seq0_len, size_y));
		printf("(%d,%d)-(%d,%d)\n", gap.i0, gap.j0, gap.i1, gap.j1);
	}
	
	cairo_line_to (cr, toPoint(j1, seq1_len, size_x), toPoint(i1, seq0_len, size_y));
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND); 
	cairo_set_line_width(cr, 2.0f);
	cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
	cairo_stroke(cr);*/
	
	my_rect_t rect;
	rect.x0 = MARGIN_LEFT;
	rect.x1 = size_x+MARGIN_LEFT;
	rect.y0 = MARGIN_TOP;
	rect.y1 = size_y+MARGIN_TOP;
	rect.i0 = i0;
	rect.i1 = i1;
	rect.j0 = j0;
	rect.j1 = j1;
	
	drawBackground(job, cr, &rect);
	
	if (job->dump_pruning) {
		drawPruningArea(job, cr, &rect);
	}
	
	drawGaps(job, cr, &rect, gaps);

	if (!job->dump_pruning) {
		drawLargestGaps(job, cr, &rect, 5);
	}
	
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	
	draw_accession_numbers(job, cr, &rect);
	
	draw_ticks(job, cr, &rect);
	
	/* Destroy the context */
	cairo_destroy (cr);
	
	/* Write the surfate to a png file */
	//cairo_surface_write_to_png (surface, "alignment.png");
	/* Destroy the surface */
	cairo_surface_destroy (surface);
}

static void drawHistogramLines(Job* job, cairo_t* cr, my_rect_t* rect,
					 vector<gap_sequence_t>* gaps) {
	cairo_set_line_width (cr, 0.1);
	
	cairo_rectangle(cr, rect->x0, rect->y0, rect->width(), rect->height());
	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	cairo_fill(cr);
	
	cairo_rectangle(cr, rect->x0, rect->y0, rect->width(), rect->height());
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_set_line_width(cr, 1);
	cairo_stroke(cr);
	
	cairo_rectangle(cr, rect->x0, rect->y0, rect->width(), rect->height());
	cairo_clip(cr);
	
	cairo_line_to (cr, rect->toX(gaps->at(0).j0), rect->toY(gaps->at(0).i0));
	//cairo_move_to (cr, rect->x0, rect->y0);
	/*for (int k=0; k<gaps->size(); k++) {
		gap_sequence_t gap = gaps->at(k);
		if (gap.i0 < rect->i0 || gap.j0 < rect->j0) continue;
		if (gap.i1 > rect->i1 || gap.j1 > rect->j1) break;
		cairo_line_to (cr, rect->toX(gap.j0), rect->toY(gap.i0));
		cairo_line_to (cr, rect->toX(gap.j1), rect->toY(gap.i1));
		//printf("DG (%d,%d)-(%d,%d) %lf\n", gap.i0, gap.j0, gap.i1, gap.j1, rect->toX(gap.j0));
	}*/
	int k=0;
	gap_sequence_t gap = gaps->at(k);
	const int WT = 100000;
	int w[WT];
	int wp=0;
	int wc=0;
	float sum=0;
	cairo_line_to (cr, rect->toX(rect->j0), rect->y1);
	for (int j=0; j<rect->j1; j++) {
		wc++;
		if (wc>WT) {
				sum -= w[wp];
				wc = WT;
		}
		if (j>=gap.j0 && j<=gap.j1) {
			w[wp] = 0;
		} else {
			w[wp] = 1;
			sum++;
		}
		wp = (wp+1)%WT;
		if (j>=gap.j1) {
			k++;
			if (k < gaps->size()) {
				gap = gaps->at(k);
			} else {
				gap.j0 = 0x7FFFFFFF;
				gap.j1 = 0x7FFFFFFF;
			}
		}
		
		//if (gap.i0 < rect->i0 || gap.j0 < rect->j0) continue;
		if (j >= rect->j0) {
			if (j % (WT/10) == 0) {
				cairo_line_to (cr, rect->toX(j), rect->y1 - sum*100/wc);
				printf("%d: %f\n", j, sum*100/wc);
			}
		}
		//cairo_line_to (cr, rect->toX(gap.j1), rect->toY(gap.i1));
		//printf("DG (%d,%d)-(%d,%d) %lf\n", gap.i0, gap.j0, gap.i1, gap.j1, rect->toX(gap.j0));
	}
	
	cairo_line_to (cr, rect->toX(rect->j1), rect->y1);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND); 
	cairo_set_line_width(cr, 2.0f);
	cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
	cairo_stroke(cr);
	
	cairo_reset_clip(cr);
}

static void drawHistogram(Job* job) {
	cairo_surface_t *surface;
	cairo_t *cr;
	
	Sequence* seq0 = &job->seq[0];
	Sequence* seq1 = &job->seq[1];
	
	int i0=job->alignment->i0;
	int j0=job->alignment->j0;
	int i1=job->alignment->i1;
	int j1=job->alignment->j1;
	int delta_i = i1-i0;
	int delta_j = j1-j0;
	
	vector<gap_sequence_t>* gaps = job->alignment->getGapSequences();
	
	int cgaps = gaps->size();
	
	const int seq0_len = seq0->getLen();
	const int seq1_len = seq1->getLen();
	
	float size_x = IMAGE_SIZE;
	float size_y = 100;
	
	/* Creating the surface */
	//surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, SIZEX, SIZEY+TEXTY);
	surface = cairo_svg_surface_create(job->alignment_svg_filename.c_str(), size_x+TEXT_HEIGHT, size_y+TEXT_HEIGHT);
	/* Creating the context */
	cr = cairo_create (surface);
	
	cairo_set_line_width (cr, 0.1);
	
	cairo_rectangle(cr, 0.0, 0.0, size_x+TEXT_HEIGHT, size_y+TEXT_HEIGHT);
	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	cairo_fill(cr);
	
	/*cairo_rectangle(cr, 0.0, 0.0, size_x, size_y);
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_set_line_width(cr, 1);
	cairo_stroke(cr);*/
	
	/*cairo_move_to (cr, toPoint(j0, seq1_len, size_x), toPoint(i0, seq0_len, size_y));
	for (int k=0; k<gaps.size(); k++) {
		gap_sequence_t gap = gaps[k];
		cairo_line_to (cr, toPoint(gap.j0, seq1_len, size_x), toPoint(gap.i0, seq0_len, size_y));
		cairo_line_to (cr, toPoint(gap.j1, seq1_len, size_x), toPoint(gap.i1, seq0_len, size_y));
		printf("(%d,%d)-(%d,%d)\n", gap.i0, gap.j0, gap.i1, gap.j1);
}

cairo_line_to (cr, toPoint(j1, seq1_len, size_x), toPoint(i1, seq0_len, size_y));
cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND); 
cairo_set_line_width(cr, 2.0f);
cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
cairo_stroke(cr);*/
	
	my_rect_t rect;
	rect.x0 = 0;
	rect.x1 = size_x;
	rect.y0 = 0;
	rect.y1 = size_y;
	rect.i0 = i0;
	rect.i1 = i1;
	rect.j0 = j0;
	rect.j1 = j1;

	drawHistogramLines(job, cr, &rect, gaps);
	
	//drawGaps(job, cr, &rect, gaps);
	
	//drawLargestGaps(job, cr, &rect, 5);
	
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	
	/*char text[500];
	cairo_text_extents_t te;
	
	cairo_set_font_size(cr, TEXT_HEIGHT/2);
	sprintf(text, "%s", job->seq1->accession.c_str());
	cairo_text_extents (cr, text, &te);
	cairo_move_to(cr, (size_x-te.width)/2, size_y+TEXT_HEIGHT/2);
	cairo_show_text(cr, text);
	cairo_set_font_size(cr, TEXT_HEIGHT/3);
	sprintf(text, "%d", j0);
	cairo_move_to(cr, 0, size_y+TEXT_HEIGHT/3);
	cairo_show_text(cr, text);
	sprintf(text, "%d", j1);
	cairo_text_extents (cr, text, &te);
	cairo_move_to(cr, size_x-te.width, size_y+TEXT_HEIGHT/3);
	cairo_show_text(cr, text);
	
	
	cairo_save (cr);
	cairo_rotate (cr, M_PI / 2.0);
	
	cairo_set_font_size(cr, TEXT_HEIGHT/2);
	sprintf(text, "%s", job->seq0->accession.c_str());
	cairo_text_extents (cr, text, &te);
	cairo_move_to(cr, (size_y-te.width)/2, -(size_x+TEXT_HEIGHT/8));
	cairo_show_text(cr, text);
	cairo_set_font_size(cr, TEXT_HEIGHT/3);
	sprintf(text, "%d", i0);
	cairo_move_to(cr, 0, -(size_x+TEXT_HEIGHT/12));
	cairo_show_text(cr, text);
	sprintf(text, "%d", i1);
	cairo_text_extents (cr, text, &te);
	cairo_move_to(cr, size_y-te.width, -(size_x+TEXT_HEIGHT/12));
	cairo_show_text(cr, text);
	
	cairo_restore (cr);*/
	
	/*cairo_set_font_size(cr, TEXT_HEIGHT/3);
	sprintf(text, "(%d,%d)", i0, j0);
	cairo_move_to(cr, TEXT_HEIGHT/3, TEXT_HEIGHT/3);
	cairo_show_text(cr, text);
	
	sprintf(text, "(%d,%d)", i1, j1);
	cairo_text_extents (cr, text, &te);
	cairo_move_to(cr, size_x-te.width-TEXT_HEIGHT/3, size_y+TEXT_HEIGHT/3);
	cairo_show_text(cr, text);*/
	
	/*int diff_i = i1-i0;
	int diff_j = j1-j0;
	int step_ticks_i = diff_i/3;
	int step_ticks_j = diff_j/3;
	step_ticks_i = pow(10, (int)(log10(step_ticks_i)));
	step_ticks_j = pow(10, (int)(log10(step_ticks_j)));
	for (int i=(i0/(step_ticks_i/10)+1)*(step_ticks_i/10); i<=i1; i+=step_ticks_i/10) {
		int len = (i%(step_ticks_i))==0?TEXT_HEIGHT/3:TEXT_HEIGHT/8;
		cairo_move_to(cr, size_x-len, rect.toY(i));
		cairo_line_to(cr, size_x, rect.toY(i));
		cairo_stroke(cr);
		cairo_move_to(cr, 0, rect.toY(i));
		cairo_line_to(cr, len, rect.toY(i));
		cairo_stroke(cr);
	}
	for (int j=(j0/(step_ticks_j/10)+1)*(step_ticks_j/10); j<=j1; j+=step_ticks_j/10) {
		int len = (j%(step_ticks_j))==0?TEXT_HEIGHT/3:TEXT_HEIGHT/8;
		cairo_move_to(cr, rect.toX(j), size_y);
		cairo_line_to(cr, rect.toX(j), size_y-len);
		cairo_stroke(cr);
		cairo_move_to(cr, rect.toX(j), 0);
		cairo_line_to(cr, rect.toX(j), len);
		cairo_stroke(cr);
	}*/
	
	
	/* Destroy the context */
	cairo_destroy (cr);
	
	/* Write the surfate to a png file */
	//cairo_surface_write_to_png (surface, "alignment.png");
	/* Destroy the surface */
	cairo_surface_destroy (surface);
}


#endif


void stage6(Job* job, int id) {
	FILE* stats = job->fopenStatistics(STAGE_6, id);
	Sequence* seq0 = job->getAlignmentParams()->getSequence(0);
	Sequence* seq1 = job->getAlignmentParams()->getSequence(1);
	job->getAlignmentParams()->printParams(stats);
	fflush(stats);
	dna_gap_open = job->getAlignmentParams()->getGapOpen();
	dna_gap_ext  = job->getAlignmentParams()->getGapExtension();
	dna_match    = job->getAlignmentParams()->getMatch();
	dna_mismatch = job->getAlignmentParams()->getMismatch();
	
	int output_format = job->stage6_output_format;
	fprintf(stats, "Output format: %d\n", output_format);	
	
	Timer timer2;
	
	int ev_start = timer2.createEvent("START");
	int ev_load_binary = timer2.createEvent("LOAD_BINARY");
	int ev_write_text = timer2.createEvent("WRITE_TEXT");
	int ev_write_svg = timer2.createEvent("WRITE_SVG");
	int ev_end = timer2.createEvent("END");

	timer2.init();

	timer2.eventRecord(ev_start);
	
	if (job->getAlignment() == NULL || !job->getAlignment()->isFinalized()) {
		fprintf(stats, "Alignment Loaded (%s)\n", job->getAlignmentBinaryFile(id).c_str());
		fflush(stats);
		Alignment* alignment = AlignmentBinaryFile::read(job->getAlignmentBinaryFile(id));
		alignment->getAlignmentParams()->getSequences();
		for (int i=0; i<alignment->getAlignmentParams()->getSequencesCount(); i++) {
			job->loadSequenceData(alignment->getAlignmentParams()->getSequence(i));
		}
		job->setAlignment(alignment);
		timer2.eventRecord(ev_load_binary);
	}

    stage6_formats[output_format].function(job->getAlignment(), job->getAlignmentTextFile(id));
	timer2.eventRecord(ev_write_text);
	
	timer2.eventRecord(ev_end);
	
	fprintf(stats, "Stage6 times:\n");
	float diff = timer2.printStatistics(stats);
	
	fprintf(stats, "        total: %.4f\n", diff);
	fclose(stats);
	
}
