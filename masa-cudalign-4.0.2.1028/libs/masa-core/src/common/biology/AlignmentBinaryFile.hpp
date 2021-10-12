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

#ifndef ALIGNMENTBINARYFILE_HPP_
#define ALIGNMENTBINARYFILE_HPP_

#include <stdio.h>
#include <string>
using namespace std;

#include "Alignment.hpp"
#include "AlignmentParams.hpp"
#include "SequenceModifiers.hpp"



class AlignmentBinaryFile {
public:
	static Alignment* read(string filename);
	static void write(string filename, Alignment* alignment);

private:
	static void fwrite_header(FILE* file);
	static void fread_header(FILE* file);

	static void fwrite_sequences(vector<SequenceInfo*> sequences, FILE* file);
	static vector<SequenceInfo*> fread_sequences(FILE* file);

	static void fwrite_alignment_params(AlignmentParams* params, FILE* file);
	static AlignmentParams* fread_alignment_params(vector<SequenceInfo*> sequences, FILE* file);

	static void fwrite_alignment_result(Alignment* alignment, FILE* file);
	static Alignment* fread_alignment_result(AlignmentParams* params, FILE* file);

	static void fwrite_flags(SequenceModifiers* modifiers, FILE* file);
	static SequenceModifiers* fread_flags(FILE* file);

	static void fwrite_array(const char* array, const int len, FILE* file);
	static void fread_array(char* array, const int len, FILE* file);

	static void fwrite_uint4_compressed(const unsigned int i, FILE* file);
	static unsigned int fread_uint4_compressed(FILE* file);

	static void fwrite_int4(const int i, FILE* file);
	static int fread_int4(FILE* file);

	static void fwrite_int2(const short i, FILE* file);
	static short fread_int2(FILE* file);

	static void fwrite_int1(const unsigned char i, FILE* file);
	static unsigned char fread_int1(FILE* file);

	static void fwrite_str(const char* str, FILE* file);
	static string fread_str(FILE* file);

	static void fwrite_gaps(vector<gap_t>* gaps, FILE* file);
	static void fread_gaps(vector<gap_t>* gaps, FILE* file);

	static void fread_dummy(const int len, FILE* file);

};

#endif /* ALIGNMENTBINARYFILE_HPP_ */
