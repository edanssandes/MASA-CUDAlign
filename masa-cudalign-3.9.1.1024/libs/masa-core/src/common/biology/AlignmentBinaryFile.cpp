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

#include "AlignmentBinaryFile.hpp"

#include <stdio.h>
#include <vector>
using namespace std;
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

#include "Constants.hpp"

#include "../../common/BlocksFile.hpp"

#define MAGIC_HEADER		"CGFF"
#define MAGIC_HEADER_LEN	4 //in bytes
#define FILE_VERSION_MAJOR	0
#define FILE_VERSION_MINOR	1


#define END_OF_FIELDS					(0)

#define FIELD_ALIGNMENT_METHOD			(1)
#define FIELD_SCORING_SYSTEM			(2)
#define FIELD_PENALTY_SYSTEM			(3)
#define FIELD_SEQUENCE_PARAMS			(4)


#define FIELD_SEQUENCE_DESCRIPTION		(1)
#define FIELD_SEQUENCE_TYPE				(2)
#define FIELD_SEQUENCE_SIZE				(3)
#define FIELD_SEQUENCE_HASH				(4)
#define FIELD_SEQUENCE_DATA_PLAIN		(5)
#define FIELD_SEQUENCE_DATA_COMPRESSED	(6)

#define FIELD_RESULT_RAW_SCORE			(1)
#define FIELD_RESULT_BIT_SCORE			(2)
#define FIELD_RESULT_E_VALUE			(3)
#define FIELD_RESULT_SCORE_STATISTICS	(4)
#define FIELD_RESULT_GAP_LIST			(5)
#define FIELD_RESULT_BLOCKS				(6)
#define FIELD_RESULT_CELLS				(7)


#define MAX_SEQUENCES 	(2)


void AlignmentBinaryFile::write(string filename, Alignment* alignment) {
    FILE* file = fopen(filename.c_str(), "wb");

    fwrite_header(file);

    AlignmentParams* params = alignment->getAlignmentParams();
    vector<Sequence*> sequences = params->getSequences();
    vector<SequenceInfo*> info;
    info.clear();
    for (int i=0; i<sequences.size(); i++) {
    	sequences[i]->getInfo()->setIndex(i);
    	info.push_back(sequences[i]->getInfo());
    }


    fwrite_sequences(info, file);
    fwrite_alignment_params(params, file);
    fwrite_alignment_result(alignment, file);

    fclose(file);
}

Alignment* AlignmentBinaryFile::read(string filename) {
	FILE* file = fopen(filename.c_str(), "rb");

	fread_header(file);

	vector<SequenceInfo*> sequences = fread_sequences(file);
	AlignmentParams* params = fread_alignment_params(sequences, file);
	Alignment* alignment = fread_alignment_result(params, file);

	fclose(file);
	return alignment;
}

void AlignmentBinaryFile::fwrite_header(FILE* file) {
	fwrite_array(MAGIC_HEADER, MAGIC_HEADER_LEN, file);
	fwrite_int1(FILE_VERSION_MAJOR, file);
	fwrite_int1(FILE_VERSION_MINOR, file);
}

void AlignmentBinaryFile::fread_header(FILE* file) {
	char header[MAGIC_HEADER_LEN];
	fread_array(header, MAGIC_HEADER_LEN, file);
	if (memcmp(header, MAGIC_HEADER, MAGIC_HEADER_LEN) != 0) {
		fprintf(stderr, "Wrong File Format. Header Error.\n");
		// TODO ERROR
		return;
	}
	int file_version_major = fread_int1(file);
	int file_version_minor = fread_int1(file);
	if (file_version_major > FILE_VERSION_MAJOR) {
		fprintf(stderr, "File Version not supported (%d.%d > %d.%d).\n",
				file_version_major, file_version_minor,
				FILE_VERSION_MAJOR, FILE_VERSION_MINOR);
		// TODO ERROR
		return;
	}
}

void AlignmentBinaryFile::fwrite_sequences(vector<SequenceInfo*> sequences, FILE* file) {
	int count = sequences.size();
	fwrite_int4(count, file);
	for (int i=0; i<count; i++) {
		SequenceInfo* seq = sequences[i];
		seq->setIndex(i);

		fwrite_int1(FIELD_SEQUENCE_DESCRIPTION, file);
		fwrite_str(seq->getDescription().c_str(), file);

		fwrite_int1(FIELD_SEQUENCE_TYPE, file);
		fwrite_int1(seq->getType(), file);

		fwrite_int1(FIELD_SEQUENCE_SIZE, file);
		fwrite_int4(seq->getSize(), file);

		fwrite_int1(END_OF_FIELDS, file);
	}
}

vector<SequenceInfo*> AlignmentBinaryFile::fread_sequences(FILE* file) {
	vector<SequenceInfo*> sequences;
	int count = fread_int4(file);
	if (count > MAX_SEQUENCES) {
		fprintf(stderr, "Sanity Check: too many sequences (%d > %d).\n", count, MAX_SEQUENCES);
		exit(0);
	}
	for (int i=0; i<count; i++) {
		SequenceInfo* info = new SequenceInfo();
		sequences.push_back(info);
		int field;
		while ((field = fread_int1(file)) != END_OF_FIELDS) {

			int field_len;
			string str = "";
			switch (field) {
				case FIELD_SEQUENCE_DESCRIPTION:
					info->setDescription(fread_str(file));
					break;
				case FIELD_SEQUENCE_TYPE:
					info->setType(fread_int1(file));
					break;
				case FIELD_SEQUENCE_SIZE:
					info->setSize(fread_int4(file));
					break;
				case FIELD_SEQUENCE_HASH:
					info->setHash(fread_str(file));
					break;
				case FIELD_SEQUENCE_DATA_PLAIN:
					field_len = fread_int4(file);
					fread_dummy(field_len, file);
					break;
				case FIELD_SEQUENCE_DATA_COMPRESSED:
					field_len = fread_int4(file);
					fread_dummy(field_len, file);
					break;
				default:
					fprintf(stderr, "Sanity Check: Unknown Field (%d).\n", field);
					exit(0);
					break;

			}
		}
	}
	return sequences;
}


void AlignmentBinaryFile::fwrite_alignment_params(AlignmentParams* params, FILE* file) {
	fwrite_int1(FIELD_ALIGNMENT_METHOD, file);
	fwrite_int1(params->getAlignmentMethod(), file);

	fwrite_int1(FIELD_SCORING_SYSTEM, file);
	fwrite_int1(params->getScoreSystem(), file);
	switch (params->getScoreSystem()) {
		case SCORE_MATCH_MISMATCH:
			fwrite_int4(params->getMatch(), file);
			fwrite_int4(params->getMismatch(), file);
			break;
		default:
			fprintf(stderr, "Sanity Check: Unknown Score System (%d).\n", params->getPenaltySystem());
			exit(0);
			break;
	}

	fwrite_int1(FIELD_PENALTY_SYSTEM, file);
	fwrite_int1(params->getPenaltySystem(), file);
	switch (params->getPenaltySystem()) {
		case PENALTY_LINEAR_GAP:
			fwrite_int4(params->getGapExtension(), file);
			break;
		case PENALTY_AFFINE_GAP:
			fwrite_int4(params->getGapOpen(), file);
			fwrite_int4(params->getGapExtension(), file);
			break;
		default:
			fprintf(stderr, "Sanity Check: Unknown Penalty System (%d).\n", params->getPenaltySystem());
			exit(0);
			break;
	}

	fwrite_int1(FIELD_SEQUENCE_PARAMS, file);
	int count = params->getSequencesCount();
	fwrite_int4(count, file);
	for (int i=0; i<count; i++) {
		SequenceInfo* sequenceInfo = params->getSequence(i)->getInfo();
		fwrite_int4(sequenceInfo->getIndex(), file);
		SequenceModifiers* modifiers = params->getSequence(i)->getModifiers();
		fwrite_flags(modifiers, file);
	}

	fwrite_int1(END_OF_FIELDS, file);
}

AlignmentParams* AlignmentBinaryFile::fread_alignment_params(vector<SequenceInfo*> sequences, FILE* file) {
	AlignmentParams* params = new AlignmentParams();
	int field;
	while ((field = fread_int1(file)) != END_OF_FIELDS) {
		switch (field) {
			case FIELD_ALIGNMENT_METHOD:
				params->setAlignmentMethod(fread_int1(file));
				break;
			case FIELD_SCORING_SYSTEM:
				params->setScoreSystem(fread_int1(file));
				switch (params->getScoreSystem()) {
					case SCORE_MATCH_MISMATCH: {
						int match = fread_int4(file);
						int mismatch = fread_int4(file);
						params->setMatchMismatchScores(match, mismatch);
						break;
					}
					case SCORE_SIMILARITY_MATRIX: {
						fprintf(stderr, "Score Matrix not supported yet.\n");
						exit(0);
						break;
					}
					default:
						fprintf(stderr, "Unknown Score System.\n");
						exit(1);
						break;
				}
				break;
			case FIELD_PENALTY_SYSTEM:
				params->setPenaltySystem(fread_int1(file));
				switch (params->getPenaltySystem()) {
					case PENALTY_LINEAR_GAP: {
						int gapOpen = 0;
						int gapExtension = fread_int4(file);
						params->setAffineGapPenalties(gapOpen, gapExtension);
						break;
					}
					case PENALTY_AFFINE_GAP: {
						int gapOpen = fread_int4(file);
						int gapExtension = fread_int4(file);
						params->setAffineGapPenalties(gapOpen, gapExtension);
						break;
					}
					default:
						fprintf(stderr, "Unknown Penalty System.\n");
						exit(1);
						break;
				}
				break;
			case FIELD_SEQUENCE_PARAMS: {
				int count = fread_int4(file);
				for (int i=0; i<count; i++) {
					int index = fread_int4(file);

					SequenceModifiers* modifiers = fread_flags(file);

					params->addSequence(new Sequence(sequences[index], modifiers));
				}
				break;
			}
			default:
				fprintf(stderr, "Sanity Check: Unknown Field (%d).\n", field);
				exit(0);
				break;

		}
	}
	return params;
}


void AlignmentBinaryFile::fwrite_alignment_result(Alignment* alignment, FILE* file) {
	fwrite_int4(1, file); // Only 1 result is supported by now
	int count = alignment->getAlignmentParams()->getSequencesCount();

	fwrite_int1(FIELD_RESULT_RAW_SCORE, file);
	fwrite_int4(alignment->getRawScore(), file);

	fwrite_int1(FIELD_RESULT_SCORE_STATISTICS, file);
	fwrite_int4(alignment->getMatches(), file);
	fwrite_int4(alignment->getMismatches(), file);
	fwrite_int4(alignment->getGapOpen(), file);
	fwrite_int4(alignment->getGapExtensions(), file);

	fwrite_int1(FIELD_RESULT_GAP_LIST, file);
	for (int i=0; i<count; i++) {
		fwrite_int4(alignment->getStart(i), file);
		fwrite_int4(alignment->getEnd(i), file);
		fwrite_gaps(alignment->getGaps(i), file);
	}

	if (!alignment->getPruningFile().empty()) {
		//FILE* pruningFile = fopen(alignment->getPruningFile().c_str(), "rb");
		fwrite_int1(FIELD_RESULT_BLOCKS, file);
		BlocksFile* blocksFile = new BlocksFile(alignment->getPruningFile());
		int bw = 512;
		int bh = 512;
		int* buffer = blocksFile->reduceData(bh, bw);
		fwrite_int4(bh, file);
		fwrite_int4(bw, file);
		for (int i=0; i<bh*bw; i++) {
			fwrite_int4(buffer[i], file);
		}
		delete blocksFile;
//		int i;
//		while (fread(&i, sizeof(int), 1, pruningFile) > 0) {
//			fwrite_int4(i, file);
//		}
//		fclose(pruningFile);
	}

	fwrite_int1(END_OF_FIELDS, file);
}

Alignment* AlignmentBinaryFile::fread_alignment_result(AlignmentParams* params, FILE* file) {
	int results = fread_int4(file);
	if (results > 1) {
		// Only 1 result is supported by now
		fprintf(stderr, "Too many results %d.\n", results);
		exit(1);
	}

	Alignment* alignment = new Alignment(params);
	int count = params->getSequencesCount();
	int field;
	while ((field = fread_int1(file)) != END_OF_FIELDS) {
		switch (field) {
			case FIELD_RESULT_RAW_SCORE:
				alignment->setRawScore(fread_int4(file));
				break;
			case FIELD_RESULT_SCORE_STATISTICS:
				alignment->setMatches(fread_int4(file));
				alignment->setMismatches(fread_int4(file));
				alignment->setGapOpen(fread_int4(file));
				alignment->setGapExtensions(fread_int4(file));
				break;
			case FIELD_RESULT_GAP_LIST:{
				for (int i=0; i<count; i++) {
					alignment->setStart(i, fread_int4(file));
					alignment->setEnd(i, fread_int4(file));
					fread_gaps(alignment->getGaps(i), file);
				}
				break;
			}
			case FIELD_RESULT_BLOCKS:{
				int h = fread_int4(file);
				int w = fread_int4(file);
				// NOT IMPLEMENTED FOR READING
				fread_dummy(h*w*sizeof(int), file);
				break;
			}
			default:
				fprintf(stderr, "Sanity Check: Unknown Field (%d).\n", field);
				exit(0);
				break;

		}
	}
	return alignment;
}

void AlignmentBinaryFile::fwrite_flags(SequenceModifiers* modifiers, FILE* file) {
	int flags = 0;
	if (modifiers->isClearN()) {
		flags |= FLAG_CLEAR_N;
	}
	if (modifiers->isComplement()) {
		flags |= FLAG_COMPLEMENT;
	}
	if (modifiers->isReverse()) {
		flags |= FLAG_REVERSE;
	}
	fwrite_int4(flags, file);
	fwrite_int4(modifiers->getTrimStart(), file);
	fwrite_int4(modifiers->getTrimEnd(), file);
}

SequenceModifiers* AlignmentBinaryFile::fread_flags(FILE* file) {
	SequenceModifiers* modifiers = new SequenceModifiers();

	int flags = fread_int4(file);
	modifiers->setClearN    ((flags & FLAG_CLEAR_N) != 0);
	modifiers->setComplement((flags & FLAG_COMPLEMENT) != 0);
	modifiers->setReverse   ((flags & FLAG_REVERSE) != 0);

	modifiers->setTrimStart(fread_int4(file));
	modifiers->setTrimEnd(fread_int4(file));

	return modifiers;
}

void AlignmentBinaryFile::fwrite_gaps(vector<gap_t>* gaps, FILE* file) {
    int count = gaps->size();
	fwrite_int4(count, file);
	//printf("Gaps[%d]\n", count);
	int last = 0;
	for (int i=0; i<count; i++) {
		int diff = (*gaps)[i].pos - last;
		last = (*gaps)[i].pos;
		//fwrite_int4((*gaps)[i].pos, file);
		//fwrite_int4((*gaps)[i].len, file);

		fwrite_uint4_compressed(diff, file);
		fwrite_uint4_compressed((*gaps)[i].len, file);

		//printf("%4d: %8d %d\n", i, (*gaps)[i].pos, (*gaps)[i].len);
		//printf("%4d: +%8d %d\n", i, diff, (*gaps)[i].len);
	}
}



void AlignmentBinaryFile::fread_gaps(vector<gap_t>* gaps, FILE* file) {
	int count = fread_int4(file);
	gaps->clear();
	//printf("Gaps[%d]\n", count);
	int last = 0;
	for (int i=0; i<count; i++) {
		gap_t gap;
		//gap.pos = fread_int4(file);
		//gap.len = fread_int4(file);

		gap.pos = last + fread_uint4_compressed(file);
		last = gap.pos;
		gap.len = fread_uint4_compressed(file);
		gaps->push_back(gap);
		//printf("%4d: %8d %d\n", i, gap.pos, gap.len);
	}
}


void AlignmentBinaryFile::fwrite_str(const char* str, FILE* file) {
    int len = strlen(str);
    fwrite_int4(len, file);
    fwrite(str, sizeof(char), len, file);
}

string AlignmentBinaryFile::fread_str(FILE* file) {
    int len = fread_int4(file);
	const int max_len = 1000;
	if (len > max_len) {
		fprintf(stderr, "Sanity Check: string too large during file read (%d > %d).\n", len, max_len);
		exit(0);
	}
	char cstr[max_len+5];
    fread(cstr, sizeof(char), len, file);
    cstr[len] = '\0';
	return string(cstr);
}









void AlignmentBinaryFile::fwrite_array(const char* array, const int len, FILE* file) {
    fwrite(array, sizeof(char), len, file);
}

void AlignmentBinaryFile::fread_array(char* array, const int len, FILE* file) {
    fread(array, sizeof(char), len, file);
}


void AlignmentBinaryFile::fwrite_uint4_compressed(const unsigned int i, FILE* file) {
	unsigned int b0 = (i & 0xF0000000)>>28;
	unsigned int b1 = (i & 0x0FE00000)>>21;
	unsigned int b2 = (i & 0x001FC000)>>14;
	unsigned int b3 = (i & 0x00003F80)>>7;
	unsigned int b4 = (i & 0x0000007F);

	if (b0 > 0) {
		fwrite_int1(0x80|b0, file);
		fwrite_int1(0x80|b1, file);
		fwrite_int1(0x80|b2, file);
		fwrite_int1(0x80|b3, file);
		fwrite_int1(b4, file);
	} else if (b1 > 0) {
		fwrite_int1(0x80|b1, file);
		fwrite_int1(0x80|b2, file);
		fwrite_int1(0x80|b3, file);
		fwrite_int1(b4, file);
	} else if (b2 > 0) {
		fwrite_int1(0x80|b2, file);
		fwrite_int1(0x80|b3, file);
		fwrite_int1(b4, file);
	} else if (b3 > 0) {
		fwrite_int1(0x80|b3, file);
		fwrite_int1(b4, file);
	} else {
		fwrite_int1(b4, file);
	}

}

unsigned int AlignmentBinaryFile::fread_uint4_compressed(FILE* file) {
	unsigned char b = fread_int1(file);
	unsigned int i = (b & 0x7F);
	while (b >= 128) {
		b = fread_int1(file);
		i <<= 7;
		i |= (b & 0x7F);
	}
	return i;
}

void AlignmentBinaryFile::fwrite_int4(const int i, FILE* file) {
	const int j = htonl(i);
    fwrite(&j, sizeof(int), 1, file);
}

int AlignmentBinaryFile::fread_int4(FILE* file) {
	int i;
	fread(&i, sizeof(int), 1, file);
	return ntohl(i);
}

void AlignmentBinaryFile::fwrite_int2(const short i, FILE* file) {
	const short j = htons(i);
    fwrite(&j, sizeof(short), 1, file);
}

short AlignmentBinaryFile::fread_int2(FILE* file) {
	short i;
	fread(&i, sizeof(short), 1, file);
	return ntohs(i);
}

void AlignmentBinaryFile::fwrite_int1(const unsigned char i, FILE* file) {
    fwrite(&i, sizeof(unsigned char), 1, file);
}

unsigned char AlignmentBinaryFile::fread_int1(FILE* file) {
	unsigned char i;
	fread(&i, sizeof(unsigned char), 1, file);
	return i;
}

void AlignmentBinaryFile::fread_dummy(const int len, FILE* file) {
	fseek(file, len, SEEK_CUR);
}



