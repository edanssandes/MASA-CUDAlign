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

#ifndef SEQUENCEINFO_HPP_
#define SEQUENCEINFO_HPP_

#include <string>
using namespace std;

class SequenceInfo {
public:
	SequenceInfo();

	bool isEqual( const SequenceInfo* other ) const;

	string getDescription() const;
	void setDescription(string description);
	string getAccession() const;
	string getFilename() const;
	void setFilename(string filename);
	string getHash() const;
	void setHash(string hash);
	int getSize() const;
	void setSize(int size);
	int getType() const;
	void setType(int type);
	char* getData() const;
	void setData(char* data);
	int getIndex() const;
	void setIndex(int index);

private:
	string description;
	int index;
	int type;
	int size;
	string hash;
	char* data;
	string filename;
	string accession;
};

#endif /* SEQUENCEINFO_HPP_ */
