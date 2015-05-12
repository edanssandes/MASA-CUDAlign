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

#ifndef LIBMASA_HPP_
#define LIBMASA_HPP_

/* libmasa includes */
#include "libmasaTypes.hpp"
#include "IManager.hpp"
#include "IAligner.hpp"
#include "IAlignerParameter.hpp"
#include "parameters/AbstractAlignerParameters.hpp"
#include "capabilities.hpp"
#include "Partition.hpp"
#include "Grid.hpp"
#include "aligners/AbstractAligner.hpp"
#include "aligners/AbstractBlockAligner.hpp"
#include "aligners/AbstractDiagonalAligner.hpp"
#include "aligners/AbstractAlignerSafe.hpp"
#include "processors/AbstractBlockProcessor.hpp"
#include "processors/CPUBlockProcessor.hpp"

/* libmasa util includes */
#include "utils/AlignerUtils.hpp"

/* libmasa block pruning classes */
#include "pruning/AbstractBlockPruning.hpp"
#include "pruning/BlockPruningDiagonal.hpp"
#include "pruning/BlockPruningGenericN2.hpp"


/**
 * Entry point for the MASA architecture. This function must be called
 * in the main procedure of the extension. The main argc/argv parameter
 * must be passed to the libmasa_entry_point in order to process the
 * command line parameters.
 *
 * @see The aligner/example/main.cpp source file contains a simple example
 * of calling the int libmasa_entry_point function.
 *
 *
 * @param argc	number of arguments
 * @param argv	command line arguments
 * @param aligner	an instance of the IAligner that will execute the
 * 				alignment procedure.
 * @param aligner_header optional text to be print in the usage information.
 * @return	exit code.
 */
int libmasa_entry_point(int argc, char** argv, IAligner* aligner, char* aligner_header=NULL);


#endif /* LIBMASA_HPP_ */
