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

#ifndef URLCELLSWRITER_HPP_
#define URLCELLSWRITER_HPP_

#include "CellsWriter.hpp"
#include <string>
using namespace std;

class URLCellsWriter: public CellsWriter {
public:
	URLCellsWriter(string url);
	virtual ~URLCellsWriter();
	virtual void close();

	virtual int write(const cell_t* buf, int len);

private:
	CellsWriter* writer;
};

#endif /* URLCELLSWRITER_HPP_ */
