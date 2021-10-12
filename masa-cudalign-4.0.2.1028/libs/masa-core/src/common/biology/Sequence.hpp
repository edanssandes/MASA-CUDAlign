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

#ifndef _SEQUENCE_HPP
#define	_SEQUENCE_HPP

#include <vector>
#include <string>
using namespace std;

#include "Constants.hpp"
#include "SequenceInfo.hpp"
#include "SequenceData.hpp"
#include "SequenceModifiers.hpp"

class Sequence {
public:
	Sequence( SequenceInfo* info, SequenceModifiers* modifiers );
	Sequence ( const Sequence* orig );
	virtual ~Sequence();

	void copyData( const Sequence* orig );
	//void setFlags( const int flags );
	//void setFileName ( const string filename );
	//void loadFile ( );
	void trim (int delta0, int delta1);
	//void setPadding(int lenth, char c);
	//void setBoundaries (int trimStart, int trimEnd);
	//void restore();

	void reverse();
	int getAbsolutePos(int relativePos) const;

	int getLen() const;
	int getTrimStart() const;
	int getTrimEnd() const;
	//int getType() const;
	//string getAccession() const;
	//string getDescription() const;

	//void setDescription(string description);
	//int getFlags() const;
	//int getOriginalLen() const;
	SequenceInfo* getInfo() const;
	void setInfo(SequenceInfo* info);
	SequenceModifiers* getModifiers() const;
	const char* getData(bool reverse = false) const;
	const char* getForwardData() const;
	const char* getReverseData() const;
	//char getPaddingChar() const;
	//int getPaddingLenght() const;
	bool isReversed() const;

	//char* data;
private:
	SequenceInfo* info;
	SequenceModifiers* modifiers;
	SequenceData* data;
	//char* original_forward_data;
	//char* original_reverse_data;
	int reverseData;
	int offset0;
	int offset1;
	int len;

	bool dataOwner;
	//int paddingLenght;
	//char paddingChar;
	//vector<char> data_vector;
	//void updateData();
	void setBoundaries(int trimStart, int trimEnd);
};

#endif	/* _SEQUENCE_HPP */

