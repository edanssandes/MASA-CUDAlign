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

#ifndef FIRSTROW_HPP_
#define FIRSTROW_HPP_

#include "SpecialRow.hpp"
#include "../io/SeekableCellsReader.hpp"

/** @brief Class that reads and stores an Special Row considering a the
 * affine gap formula in the first row.
 *
 * This class is the implementation of a SpecialRow that considers the formula
 * of the affine gap function in the first row. It does not need to store the
 * cells in memory, since it can reproduce the cells using the
 * affine gap formula.
 *
 * This class works only in read mode. Thus, it contains many methods that
 * does nothing, but we need to implement them since the abstract SpecialRow
 * superclass contains virtual methods.
 *
 */
class FirstRow : public SpecialRow {
public:
	/**
	 * Default contructor.
	 */
	FirstRow();

	/**
	 * Destructor.
	 */
	virtual ~FirstRow();

	/**
	 * This method does nothing, but we need to implement it since the
	 * abstract SpecialRow superclass contains this virtual method.
	 */
	virtual void close();

	/**
	 * This method does nothing, but we need to implement it since the
	 * abstract SpecialRow superclass contains this virtual method.
	 */
	virtual void truncateRow(int size);

	/**
	 * Defines the affine gap function parameters.
	 *
	 * @param score_params the affine gap function parameters.
	 * @param firstRowGapped If true, the affine gap is used in the entire row
	 * 		(global or semi-global alignment scenarios). Otherwise, we consider
	 * 		that the first row if filled with zeroes (local alignment scenario).
	 */
	//void setParams(const score_params_t* score_params, 	bool firstRowGapped);
	void setCellsReader(SeekableCellsReader* reader);

private:
	/** the affine gap function parameters */
//	const score_params_t* score_params;
//
//	/** indicates if the first row considers the affine gap function or not */
//	bool firstRowGapped;
	SeekableCellsReader* reader;


	/**
	 * This method does nothing, but we need to implement it since the
	 * abstract SpecialRow superclass contains this virtual method.
	 */
	virtual void initialize(bool readOnly, int length);

	/**
	 * This method does nothing, but we need to implement it since the
	 * abstract SpecialRow superclass contains this virtual method.
	 * @return always zero.
	 */
	virtual int write(const cell_t* buf, int offset, int len);

	/**
	 * This method does nothing, but we need to implement it since the
	 * abstract SpecialRow superclass contains this virtual method.
	 */
	virtual int read(cell_t* buf, int offset, int len);
};

#endif /* FIRSTROW_HPP_ */
