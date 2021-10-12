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

#ifndef BUFFEREDCELLSREADER_HPP_
#define BUFFEREDCELLSREADER_HPP_

#include "CellsReader.hpp"
#include "BufferedStream.hpp"
#include "FileCellsReader.hpp"
#include "FileCellsWriter.hpp"

class BufferedCellsReader: public SeekableCellsReader, public BufferedStream {
public:
	BufferedCellsReader(CellsReader* reader, int bufferLimit);
	virtual ~BufferedCellsReader();
	virtual void close();

	virtual int getType();
	virtual int read(cell_t* buf, int len);
	virtual void seek(int position);
	virtual int getOffset();
private:
	CellsReader* reader;
	int offset;

    void bufferLoop();
};

#endif /* BUFFEREDCELLSREADER_HPP_ */
