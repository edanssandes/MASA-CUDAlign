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

#ifndef ALIGNERUTILS_HPP_
#define ALIGNERUTILS_HPP_

#include "../libmasa.hpp"

class AlignerUtils {
public:
	static void splitBlocksEvenly(int* pos, int j0, int j1, int count);
	static match_result_t matchColumn(const cell_t* buffer, const cell_t* base, int len, int goalScore, int gap_open_penalty);
};

#endif /* ALIGNERUTILS_HPP_ */
