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

#include "AbstractAlignerParameters.hpp"

#include <stdio.h>
#include <string.h>

/**
 * Initializes the inner structures of the object.
 */
AbstractAlignerParameters::AbstractAlignerParameters() {
	forkId = NOT_FORKED_INSTANCE;
}

/**
 * Destruct the allocated structures.
 */
AbstractAlignerParameters::~AbstractAlignerParameters() {
}

/*
 * @see definition on header file
 */
const char* AbstractAlignerParameters::getLastError() const {
	return lastError;
}

/*
 * @see definition on header file
 */
void AbstractAlignerParameters::printFormattedUsage(const char* header, const char* text) {
    printf ( "\033[1m%s:\033[0m\n", header);
    printf ( "%s\n\n", text );
}

/*
 * @see definition on header file
 */
int AbstractAlignerParameters::callGetOpt(int argc, char** argv, option* arguments) {
	optind--;
	int ret = getopt_long ( argc, argv, ":", arguments, NULL );
	if (ret == '?') {
		return ARGUMENT_ERROR_NOT_FOUND;
	} else if (ret == ':') {
		return ARGUMENT_ERROR_NO_OPTION;
	}
	return ret;
}

/*
 * @see definition on header file
 */
int AbstractAlignerParameters::getForkId() const {
	return forkId;
}

/*
 * @see definition on header file
 */
void AbstractAlignerParameters::setForkId(int forkId) {
	this->forkId = forkId;
}

/*
 * @see definition on header file
 */
void AbstractAlignerParameters::setLastError(const char* error) {
	strncpy(lastError, error, sizeof(lastError)-1);
	lastError[sizeof(lastError)-1] = '\0';
}



