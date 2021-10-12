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

#ifndef SPECIALROWRAM_HPP_
#define SPECIALROWRAM_HPP_

#include "SpecialRow.hpp"

/** @brief Class that reads and stores an Special Row in the RAM memory.
 *
 * This class is the implementation of a SpecialRow that stores the cells
 * in RAM memory. Consider that the row stored by this class is not persistent,
 * so it is not susceptible to chekpoint restore.
 *
 */
class SpecialRowRAM : public SpecialRow {
public:
	/**
	 * Creates a new Special Row associated to the RAM memory.
	 * @param id the rowId
	 */
	SpecialRowRAM(int id);

	/**
	 * Dispose the RAM memory allocated to this object.
	 */
	virtual ~SpecialRowRAM();

	/**
	 * Does nothing.
	 */
	virtual void close();

	/**
	 * Truncates the allocated memory.
	 *
	 * @param size the number of cells to keep in the memory.
	 */
	virtual void truncateRow(int size);

private:
	/** Allocated memory. */
	cell_t* row;

	/** Length of the allocated memory */
	int length;

	/**
	 * Allocate the initial structure for this row
	 *
	 * @param readOnly true if it must be opened for read mode, false otherwise.
	 * @param length the initial size of the row in memory.
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
};

#endif /* SPECIALROWRAM_HPP_ */
