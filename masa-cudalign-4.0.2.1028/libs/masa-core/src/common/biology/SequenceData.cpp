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

#include "SequenceData.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string>
using namespace std;

#include "Constants.hpp"
#include "SequenceModifiers.hpp"

/*SequenceData::SequenceData(char* data, int size, SequenceModifiers* modifiers) {
	this->modifiers = modifiers;
	this->forwardData = data;
	this->originalSize = size;
	this->size = size;
	this->reverseData = createReverseData(this->forwardData, this->size);
}*/

SequenceData::SequenceData(string filename, SequenceModifiers* modifiers) {
	this->modifiers = modifiers;
	loadFile(filename);
	if (this->modifiers->getTrimStart() == 0) {
		this->modifiers->setTrimStart(1);
	}
	if (this->modifiers->getTrimEnd() == 0) {
		this->modifiers->setTrimEnd(this->size);
	}
}

SequenceData::~SequenceData() {
	free(forwardData);
	forwardData = NULL;
	free(reverseData);
	reverseData = NULL;
}

char* SequenceData::createReverseData(char* forwardData, int size) {
	char* reverseData = (char*) (malloc(size + 1));
	for (int i = 0; i < size; i++) {
		reverseData[size - 1 - i] = forwardData[i];
	}
	reverseData[size] = '\0';
	return reverseData;
}

void SequenceData::loadFile(string filename) {
    FILE* file = fopen(filename.c_str(), "rt");
    if (file == NULL) {
		fprintf(stderr, "Error opening fasta file: %s\n", filename.c_str());
		exit(1);
    }

    fseek(file, 0L, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    char line[500];
    fgets(line, sizeof(line), file);
    description = line;

    this->forwardData = (char*)(malloc(fileSize+1));


	char complement_map[256];
	for (int i=0; i<256; i++) {
		complement_map[i] = toupper(i);
	}
	if (modifiers->isComplement()) {
		complement_map['A'] = complement_map['a'] = 'T';
		complement_map['T'] = complement_map['t'] = 'A';
		complement_map['C'] = complement_map['c'] = 'G';
		complement_map['G'] = complement_map['g'] = 'C';
	}
	if (modifiers->isClearN()) {
		complement_map['N'] = complement_map['n'] = 'n'; // lower case
	}

	this->size = 0;
	this->originalSize = 0;
    int i;
    while ((i=fgetc(file)) != EOF) {
        if (i == '\r' || i == '\n' || i == ' ') continue;
        this->originalSize++;
		//if (modifiers->isClearN() && ((char)i == 'N' || (char)i == 'n')) continue;
		this->forwardData[this->size] = complement_map[(char)i];
		this->size++;
    }
    this->forwardData[this->size] = '\0';
    this->forwardData = (char*)realloc(this->forwardData, this->size+1);
    this->reverseData = createReverseData(this->forwardData, this->size);

    fclose(file);
}

char* SequenceData::getForwardData() const {
	return forwardData;
}

char* SequenceData::getReverseData() const {
	return reverseData;
}

int SequenceData::getSize() const {
	return size;
}

string SequenceData::getDescription() const
{
    return description;
}

int SequenceData::getOriginalSize() const {
	return originalSize;
}

