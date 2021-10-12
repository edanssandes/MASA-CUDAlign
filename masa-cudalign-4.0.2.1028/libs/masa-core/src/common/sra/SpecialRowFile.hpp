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

#ifndef SPECIALROWFILE_HPP_
#define SPECIALROWFILE_HPP_

#include "SpecialRow.hpp"

/** @brief Class that reads and stores an Special Row in the disk.
 *
 * This class is the implementation of a SpecialRow that stores the cells
 * in a file saved in the disk.
 *
 * While the special row is in write mode, a temporary file (*.tmp) store the
 * cells. As soon as the row turn to read mode, the temporary file is closed
 * and renamed to the definitive name.
 */
class SpecialRowFile : public SpecialRow {
public:
	/**
	 * Creates a new Special Row associated with a given file.
	 *
	 * @param path the path to save this file.
	 * @param filename the name of this file. The rowId is extracted from this
	 *   name. If the filename is not a valid row name, rowId is set to -1.
	 */
	SpecialRowFile(string* path, string filename);

	/**
	 * Creates a new Special Row associated with a given rowId.
	 *
	 * @param path the path to save this file.
	 * @param id the rowId of this file. The filename is associated to this id.
	 * 	The rowId must be relative to the partition, so the rowId 0 is the first
	 * 	row of the partition.
	 */
	SpecialRowFile(string* path, int id);

	/**
	 * Destroys any allocated resource previously allocated by this object.
	 */
	virtual ~SpecialRowFile();

	/**
	 * Closes the file.
	 */
	virtual void close();

	/**
	 * Truncates the file.
	 *
	 * @param size the number of cells to keep in the file.
	 */
	virtual void truncateRow(int size);

private:
	/** Dynamic path name of the partition. */
	string* path; // this string changes when the partition changes its name.

	/** File name basic prefix*/
	string filename;

	/** Opened file descriptor */
	FILE* file;

	/**
	 * Opens the file for read or write mode.
	 * @param readOnly true if it must be opened for read mode, false otherwise.
	 * @param length the initial size of the file.
	 */
	virtual void initialize(bool readOnly, int length);

	/*
	 * @see description in superclass header.
	 */
	virtual int write(const cell_t* buf, int offset, int len);

	/*
	 * @see description in superclass header.
	 */
	virtual int read(cell_t* buf, int offset, int len);

	/**
	 * Returns the complete filename (with path) of the special row.
	 * @param temp indicates if the filename is temporary or definitive.
	 */
	string getFullFilename(bool temp);
};

#endif /* SPECIALROWFILE_HPP_ */
