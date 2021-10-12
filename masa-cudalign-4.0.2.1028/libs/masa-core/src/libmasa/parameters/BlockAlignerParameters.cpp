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

#include "BlockAlignerParameters.hpp"

#include <stdio.h>
#include <unistd.h>

#include <sstream>
using namespace std;


#include "../aligners/AbstractBlockAligner.hpp"

#define AUTO_GRID_SIZE			(-1)

#define DEFAULT_BLOCK_SIZE		 0
#define DEFAULT_BLOCK_SIZE_STR	"0"

#define DEFAULT_GRID_SIZE		 AUTO_GRID_SIZE
#define DEFAULT_GRID_SIZE_STR	"Auto"

/**
 * Usage Strings
 */
#define USAGE_HEADER	"Specific Parameters"
#define USAGE "\
--block-width=W              Defines that each Block has H cells vertically. Default: "DEFAULT_BLOCK_SIZE_STR".\n\
--block-height=H             Defines that each Block has W cells horizontally. Default: "DEFAULT_BLOCK_SIZE_STR".\n\
--block-size=H,W             Defines the dimensions of each block.\n\
--grid-width=W               Divides the Grid in H rows of blocks. Default: "DEFAULT_GRID_SIZE_STR".\n\
--grid-height=H              Divides the Grid in W columns of blocks. Default: "DEFAULT_GRID_SIZE_STR".\n\
--grid-size=H,W              Defines the dimensions of the grid.\n\
"

/**
 * getopt arguments
 */
#define ARG_GRID_WIDTH   0x1001
#define ARG_GRID_HEIGHT  0x1002
#define ARG_BLOCK_HEIGHT 0x1003
#define ARG_BLOCK_WIDTH  0x1004
#define ARG_GRID_SIZE    0x1005
#define ARG_BLOCK_SIZE   0x1006

static struct option long_options[] = {
        {"block-height",     required_argument,      0, ARG_BLOCK_HEIGHT},
        {"block-width",      required_argument,      0, ARG_BLOCK_WIDTH},
        {"grid-height",      required_argument,      0, ARG_GRID_HEIGHT},
        {"grid-width",       required_argument,      0, ARG_GRID_WIDTH},
        {"grid-size",        required_argument,      0, ARG_GRID_SIZE},
        {"block-size",       required_argument,      0, ARG_BLOCK_SIZE},
        {0, 0, 0, 0}
    };

/*
 * Constructor. Initializates the inner structures.
 */
BlockAlignerParameters::BlockAlignerParameters() {
	gridWidth = DEFAULT_GRID_SIZE;
	gridHeight = DEFAULT_GRID_SIZE;
	blockWidth = DEFAULT_BLOCK_SIZE;
	blockHeight = DEFAULT_BLOCK_SIZE;
}

void BlockAlignerParameters::printUsage() const {
	AbstractAlignerParameters::printFormattedUsage(USAGE_HEADER, USAGE);
}

/*
 * Destructor
 */
BlockAlignerParameters::~BlockAlignerParameters() {
}

int BlockAlignerParameters::processArgument(int argc, char** argv) {

	int ret = AbstractAlignerParameters::callGetOpt(argc, argv, long_options);
	switch ( ret ) {
		case ARG_GRID_SIZE:
			sscanf ( optarg, "%d,%d", &gridHeight,&gridWidth);
			break;
		case ARG_GRID_WIDTH:
			sscanf ( optarg, "%d", &gridWidth );
			break;
		case ARG_GRID_HEIGHT:
			sscanf ( optarg, "%d", &gridHeight );
			break;
		case ARG_BLOCK_SIZE:
			sscanf ( optarg, "%d,%d", &blockHeight,&blockWidth);
			break;
		case ARG_BLOCK_WIDTH:
			sscanf ( optarg, "%d", &blockWidth);
			break;
		case ARG_BLOCK_HEIGHT:
			sscanf ( optarg, "%d", &blockHeight);
			break;
		default:
			return ret;
	}

	if ( gridHeight > MAX_GRID_SIZE || gridWidth > MAX_GRID_SIZE
			|| (gridHeight < MIN_GRID_SIZE && gridHeight != AUTO_GRID_SIZE)
			|| (gridWidth < MIN_GRID_SIZE && gridWidth != AUTO_GRID_SIZE)) {
		stringstream out;
		out << "Grid dimensions must be in range [" << MIN_GRID_SIZE << ".."  << MAX_GRID_SIZE <<  "].";
		setLastError(out.str().c_str());
		return ARGUMENT_ERROR;
	}
	if ( blockHeight > MAX_BLOCK_SIZE || blockWidth > MAX_BLOCK_SIZE
			|| (blockHeight < MIN_BLOCK_SIZE && blockHeight != 0)
			|| (blockWidth < MIN_BLOCK_SIZE && blockWidth != 0)) {
		stringstream out;
		out << "Block dimensions must be in range [" << MIN_BLOCK_SIZE << ".."  << MAX_BLOCK_SIZE <<  "].";
		setLastError(out.str().c_str());
		return ARGUMENT_ERROR;
	}

	return ARGUMENT_OK;
}

int BlockAlignerParameters::getBlockWidth() const {
	return blockWidth;
}

int BlockAlignerParameters::getGridWidth() const {
	return gridWidth;
}

int BlockAlignerParameters::getBlockHeight() const {
	return blockHeight;
}

int BlockAlignerParameters::getGridHeight() const {
	return gridHeight;
}
