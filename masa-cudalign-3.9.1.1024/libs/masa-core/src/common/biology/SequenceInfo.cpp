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

#include "SequenceInfo.hpp"
#include "Constants.hpp"

#include <stdio.h>

SequenceInfo::SequenceInfo() {
	// TODO detectar automaticamente
	this->type = SEQUENCE_TYPE_DNA;
}

bool SequenceInfo::isEqual(const SequenceInfo* other) const {
	bool hashEqual = (hash.length() == 0 || other->hash.length() ==0 || hash == other->hash);
	bool sizeEqual = (size == 0 || other->size == 0 || size == other->size);
	bool descriptionEqual = (description.length() == 0 || other->description.length() ==0 || description == other->description);
	bool typeEqual = (type == SEQUENCE_TYPE_UNKNOWN || other->type == SEQUENCE_TYPE_UNKNOWN || type == other->type);
	bool fileEqual = (filename.length() == 0 || other->filename.length() ==0 || filename == other->filename);

	//printf("SequenceInfo::operator == %d %d %d %d %d\n", hashEqual , sizeEqual , descriptionEqual , typeEqual , fileEqual);

	return hashEqual && sizeEqual && descriptionEqual && typeEqual && fileEqual;
}

char* SequenceInfo::getData() const {
	return data;
}

void SequenceInfo::setData(char* data) {
	this->data = data;
}

string SequenceInfo::getDescription() const {
	return description;
}

void SequenceInfo::setDescription(string description) {
	this->description = description;

	if (description[0] == '>') {
		this->description = description.substr(1);
	} else {
		this->description = description;
	}
	int nl = this->description.find('\n');
	if (nl != string::npos) {
		this->description = this->description.substr(0, nl);
	}

    int pos = -1;
    int lastpos = 0;
	for (int i=0; i<4; i++) {
		lastpos = pos;
	    pos = description.find('|', lastpos+1);
	    if (pos == description.length()) break;
	}
	if (pos == string::npos) {
		accession = description;
	} else {
		accession = description.substr(lastpos+1, pos-lastpos-1);
	}
}

string SequenceInfo::getAccession() const {
	return accession;
}

string SequenceInfo::getFilename() const {
	return filename;
}

void SequenceInfo::setFilename(string filename) {
	this->filename = filename;
}

string SequenceInfo::getHash() const {
	return hash;
}

void SequenceInfo::setHash(string hash) {
	this->hash = hash;
}

int SequenceInfo::getSize() const {
	return size;
}

void SequenceInfo::setSize(int size) {
	this->size = size;
}

int SequenceInfo::getType() const {
	return type;
}

void SequenceInfo::setType(int type) {
	this->type = type;
}

int SequenceInfo::getIndex() const {
	return index;
}

void SequenceInfo::setIndex(int index) {
	this->index = index;
}


