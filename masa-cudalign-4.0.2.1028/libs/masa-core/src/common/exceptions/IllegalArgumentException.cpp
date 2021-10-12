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

#include "IllegalArgumentException.hpp"

#include <string.h>

IllegalArgumentException::IllegalArgumentException(const char* err, const char* arg) {
	this->err = string(err);
	this->arg = string(arg);
}

const char* IllegalArgumentException::what() const throw () {
	string str = "Parameter error: ";
	if (arg.length() != 0) {
		str += arg + "\n";
		str += "                 ";
	}
	if (err.length() != 0) {
		str += err;
	}
	str += "\n";
	return str.c_str();
}

string IllegalArgumentException::getArg() const {
	return arg;
}

string IllegalArgumentException::getErr() const {
	return err;
}

