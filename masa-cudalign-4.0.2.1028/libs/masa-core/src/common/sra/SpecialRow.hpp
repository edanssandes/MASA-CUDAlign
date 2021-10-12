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

#ifndef SPECIALROW_HPP_
#define SPECIALROW_HPP_

#include <stdio.h>

#include <string>
using namespace std;

#include "../io/SeekableCellsReader.hpp"
#include "../io/CellsWriter.hpp"
#include "../../libmasa/libmasa.hpp"

/** @brief Abstract class that represents an Special Row.
 *
 * A Special Row is a row (or column) stored to be used in the further stages.
 * This class permit that subclasses store the special row in
 * any possible area (e.g.: disk, memory, database, etc.).
 *
 */
class SpecialRow : public SeekableCellsReader, public CellsWriter {
public:
	/**
	 * Default Constructor.
	 */
	SpecialRow();

	/**
	 * Default Destructor.
	 */
	virtual ~SpecialRow();

	/**
	 * Function that sorts the special rows by ID.
	 */
    static int sortById(SpecialRow* a, SpecialRow* b);

    /**
     * Opens the special rows for read-only or write-only.
     *
     * @param readOnly if true, indicates that the row will be used
     * only for read access. Otherwise, the row will only be used
     * for write access.
     * @param length the maximum length of this row.
     */
	void open(bool readOnly, int length=0);

	/**
	 * Writes data in the row serially.
	 *
	 * @param buf vector containing the cells to be appended to the row.
	 * @param len length of the vector.
	 * @return The number of cells written.
	 */
	virtual int write(const cell_t* buf, int len);

	/**
	 * Reads data from the row serially. The read access is done in the
	 * reverse direction, from the end to the beginning. Before reading,
	 * the SpecialRow::setOffset must defines the start position to be read.
	 *
	 * @param buf vector that will receive the cells read from the row.
	 * @param len length of the vector.
	 * @return The number of cells read.
	 */
	virtual int read(cell_t* buf, int len);


	virtual int getType();

	/**
	 * Defines the next position to be read.
	 * @param offset The reading position.
	 */
	void seek(int offset);

	/**
	 * Returns the current reading position.
	 * @return the index of the next cell to be read.
	 */
	int getOffset();


	/**
	 * Returns the ID of the row.
	 * @return the number of the row (i-coordinate).
	 */
	int getId();

	/**
	 * Closes the special row for writing.
	 */
	virtual void close() = 0;

	/**
	 * Truncate the size of the row. If the size is zero, the row must
	 * be completely deleted. Each subclass must implement this
	 * method with specific code.
	 *
	 * @param size the number of cells to keep after truncation.
	 */
	virtual void truncateRow(int size) = 0;

protected:

	/**
	 * Defines the ID of the row, i.e., the number of the row it represents.
	 * @param id the number of the row (i-coordinate).
	 */
	void setId(int id);

private:
	/** The number of the row that this object represents */
	int id;

	/** Current reading position */
	int offset;

	/** Indicates if the row is in reading or writing mode */
	bool readOnly;

	/**
	 * Initializes the row with specific data.
	 *
	 * @param readOnly if true, indicates that the row will be used
     * only for read access. Otherwise, the row will only be used
     * for write access. Each subclass must implement this
	 * method with specific code.
	 * @param length the maximum length of this row. The subclass
	 * may consider this as the initial size, for faster allocation.
	 */
	virtual void initialize(bool readOnly, int length=0) = 0;

	/**
	 * Stores the vector in a given offset. It is guaranteed that the
	 * offset is serially incremented. Each subclass must implement this
	 * method with specific code.
	 *
	 * @param buf vector containing the cells to be saved.
	 * @param offset the position that the vector must be stored.
	 * @param len length of the vector.
	 * @return The number of cells stored.
	 */
	virtual int write(const cell_t* buf, int offset, int len) = 0;

	/**
	 * Stores the vector in a given offset. It is guaranteed that the
	 * offset is serially decremented. Each subclass must implement this
	 * method with specific code.
	 *
	 * @param buf vector containing the cells to be read. The data read will
	 * 	be stored in the first byte of this vector.
	 * @param offset the position that the data must be read.
	 * @param len length of the vector.
	 * @return The number of cells read.
	 */
	virtual int read(cell_t* buf, int offset, int len) = 0;

};

#endif /* SPECIALROW_HPP_ */
