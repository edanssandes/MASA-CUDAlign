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

#include "SpecialRowFile.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

#define DEBUG (0)

/*
 * @see description on header file
 */
SpecialRowFile::SpecialRowFile(string* path, string filename)
{
	this->path = path;
	this->filename = filename;
	this->file = NULL;
	int id = -1;
	if (filename.length() == 8) {
		sscanf(filename.c_str(), "%X", &id);
	} else if (filename.length() == 8 + 4 && filename.compare(filename.length()-4, 4, ".tmp") == 0) {
		string fullFile = ((*path) + "/" + filename);
		//printf("Removing Temporary File: %s\n", fullFile.c_str());
		remove(fullFile.c_str());
	}
	setId(id);
}

/*
 * @see description on header file
 */
SpecialRowFile::SpecialRowFile(string* path, int id)
{
	this->path = path;
	this->file = NULL;
	setId(id);

	char str[256];
	sprintf(str, "%08X", id);
	this->filename = string(str);

}

/*
 * @see description on header file
 */
SpecialRowFile::~SpecialRowFile()
{
	close();
}

/*
 * @see description on header file
 */
void SpecialRowFile::initialize(bool readOnly, int length) {
	string filename = getFullFilename(!readOnly);
	file = fopen(filename.c_str(), readOnly ? "rb" : "wb");
	if (file == NULL) {
		fprintf(stderr, "Could not %s special row: %s\n", readOnly?"open":"create",
				filename.c_str());
		perror("fopen()");
		exit(1);
	}
	if (!readOnly) {
		truncate(filename.c_str(), length*sizeof(cell_t));
	}
}

/*
 * @see description on header file
 */
void SpecialRowFile::close() {
	if (file != NULL) {
		fclose(file);
		file = NULL;
		string filenameTmp = getFullFilename(true);
		string filenameDef = getFullFilename(false);
		rename(filenameTmp.c_str(), filenameDef.c_str());
		//printf("Closed %s\n", filename.c_str());
	}
}

/*
 * @see description on header file
 */
void SpecialRowFile::truncateRow(int size) {
	string filenameDef = getFullFilename(false);
	if (size == 0) {
		remove(filenameDef.c_str());
		//printf("Removed %s\n", filenameDef.c_str());
	} else {
		struct stat st;
		stat(filenameDef.c_str(), &st);
		if (size*sizeof(cell_t) < st.st_size) {
			truncate(filenameDef.c_str(), size*sizeof(cell_t));
			//printf("Truncated[%d bytes] %s\n", st.st_size - size*sizeof(cell_t), filenameDef.c_str());
		}
	}
}

/*
 * @see description on header file
 */
int SpecialRowFile::write(const cell_t* buf, int offset, int len) {
	return fwrite(buf, sizeof(cell_t), len, file);
}

/*
 * @see description on header file
 */
int SpecialRowFile::read(cell_t* buf, int offset, int len) {
	if (DEBUG) printf("SpecialRowFile::read(%p, %d, %d): %s\n", buf, offset, len, filename.c_str());
	fseek(file, offset*sizeof(cell_t), SEEK_SET);
	int pos = 0;
	while (pos<len) {
		int ret = fread(buf, sizeof(cell_t), len-pos, file);
		if (ret == 0) {
			return pos;
		}
		pos += ret;
	}
	return pos;
}

string SpecialRowFile::getFullFilename(bool temp) {
	if (temp) {
		return (*path) + "/" + filename + ".tmp";
	} else {
		return (*path) + "/" + filename;
	}
}
