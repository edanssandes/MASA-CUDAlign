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

#ifndef SEQUENCEMODIFIERS_HPP_
#define SEQUENCEMODIFIERS_HPP_

class SequenceModifiers {
public:
	bool isCompatible(const SequenceModifiers* other) const;

	int getTrimEnd() const;
	void setTrimEnd(int trimEnd);
	int getTrimStart() const;
	void setTrimStart(int trimStart);
	bool isClearN() const;
	void setClearN(bool clearN);
	bool isComplement() const;
	void setComplement(bool complement);
	bool isReverse() const;
	void setReverse(bool reverse);

private:
	bool clear_n;
	bool complement;
	bool reverse;

	int trimStart;
	int trimEnd;
};

#endif /* SEQUENCEMODIFIERS_HPP_ */
