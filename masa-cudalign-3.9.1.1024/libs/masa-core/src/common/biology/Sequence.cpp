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

#include "Sequence.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


Sequence::Sequence( SequenceInfo* info, SequenceModifiers* modifiers ) {
	this->info = info;
	this->modifiers = modifiers;
	this->dataOwner = false;
	if (info->getFilename() != "") {
		this->data = new SequenceData(info->getFilename(), modifiers);
		this->dataOwner = true;
	} else if (info->getData() != NULL) {
		// TODO
		//this->data = new SequenceData(info->getData(), info->getSize(), modifiers);
		this->data = NULL;
	} else {
		this->data = NULL;
	}

	if (this->data != NULL) {
		this->info->setDescription(this->data->getDescription());
		this->info->setSize(this->data->getOriginalSize());
		this->len = this->data->getSize();
	} else {
		this->len = 0;
	}
	setBoundaries(modifiers->getTrimStart(), modifiers->getTrimEnd());
	this->reverseData = modifiers->isReverse();
	//this->paddingLenght = 0;
	//this->paddingChar = '\0';
}

Sequence::Sequence(const Sequence* orig) {
	this->info          = orig->info;
	this->modifiers     = orig->modifiers;
	this->data          = orig->data;
	this->len           = orig->len;
	//this->paddingChar   = orig->paddingChar;
	//this->paddingLenght = orig->paddingLenght;
	this->offset0       = orig->offset0;
	this->offset1       = orig->offset1;
	this->reverseData   = orig->reverseData;
	this->dataOwner 	= false;
}

Sequence::~Sequence() {
	if (dataOwner) {
		delete data;
	}
	// TODO desalocar os dados. Atenção para as duplicações de ponteiros
}


void Sequence::copyData(const Sequence* orig) {
	this->dataOwner = false;
	if (this->modifiers->isCompatible(orig->modifiers)) {
		this->data = orig->data;
	} else if (orig->info->getFilename() != "") {
		this->data = new SequenceData(orig->info->getFilename(), modifiers);
		this->dataOwner = true;
	} else if (orig->info->getData() != NULL) {
		// TODO
		//this->data = new SequenceData(orig->info->getData(), modifiers);
		this->data = NULL;
	} else {
		this->data = NULL;
	}
}

int Sequence::getLen() const {
	//fprintf(stderr, "LEN+PADDING: %d+%d=%d\n", len, padding, len + padding);
    return len;
}

void Sequence::reverse() {
	this->reverseData = !this->reverseData;
}

void Sequence::trim (int delta0, int delta1) {
	if (delta0 <= 0) {
		delta0 = 1;
	}
	if (delta1 <= 0) {
		delta1 = len;
	}
	setBoundaries(offset0 + (delta0-1), offset1 - (len-delta1));
}


/*void Sequence::setPadding(int length, char c) {
	this->paddingLenght = length;
	this->paddingChar = c;
}

char Sequence::getPaddingChar() const {
	return paddingChar;
}

int Sequence::getPaddingLenght() const {
	return paddingLenght;
}*/

void Sequence::setBoundaries(int trimStart, int trimEnd) {
	if (trimStart <= 0) {
		trimStart = 1;
	}
	if (trimEnd <= 0) {
		trimEnd = data->getSize();
	}
	offset0 = trimStart;
	offset1 = trimEnd;
	len = trimEnd - trimStart + 1;
	printf("TRIM: %d..%d (%d)\n", offset0, offset1, len);
}

// offset: 1-based;  relativePos: 1-based
int Sequence::getAbsolutePos(int relativePos) const {
	if (reverseData) {
		return getInfo()->getSize()+1-relativePos;
	} else {
		return relativePos;
	}
}

bool Sequence::isReversed() const {
	return reverseData;
}

SequenceInfo* Sequence::getInfo() const {
	return info;
}

void Sequence::setInfo(SequenceInfo* info) {
	this->info = info;
}

SequenceModifiers* Sequence::getModifiers() const {
	return modifiers;
}

int Sequence::getTrimStart() const {
	return offset0;
}

int Sequence::getTrimEnd() const {
	return offset1;
}

const char* Sequence::getData(bool reverse) const {
	if (reverseData ^ reverse) {
		return this->data->getReverseData();// + (this->data->getSize()-offset1);
	} else {
		return this->data->getForwardData();// + (offset0-1);
	}
}

const char* Sequence::getForwardData() const {
	return this->data->getForwardData();
}


const char* Sequence::getReverseData() const {
	return this->data->getReverseData();
}






