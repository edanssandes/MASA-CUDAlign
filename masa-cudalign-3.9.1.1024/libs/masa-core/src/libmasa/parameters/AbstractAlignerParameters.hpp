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

#ifndef ABSTRACTALIGNERPARAMETER_HPP_
#define ABSTRACTALIGNERPARAMETER_HPP_

#include <stdio.h>
#include <getopt.h>
#include "../IAlignerParameter.hpp"


/**
 * The customized parameters of a MASA Extension, used for receiving,
 * manipulating and customizing command line parameters. The
 * AbstractAlignerParameters class implements the basic operations of the
 * IAlignerParameter. Extend this class to implement Aligner specific parameters.
 *
 * @see IAlignerParameter
 * @see <a href="http://www.gnu.org/software/libc/manual/html_node/Getopt.html">GNU getopt C library</a> (External link)
 */
class AbstractAlignerParameters : public IAlignerParameters {


public:
	/**
	 * Constructor
	 */
	AbstractAlignerParameters();

	/**
	 * Destructor
	 */
	virtual ~AbstractAlignerParameters();

	/* Implemented virtual methods inherited from IAligner */
	virtual const char* getLastError() const;
	virtual int getForkId() const;
	virtual void setForkId(int forkId);

protected:

	/**
	 * Defines the number of processes that may be forked using the "--fork"
	 * command line parameter.
	 *
	 * @param forkCount the maximum number of forked processes.
	 * @param forkWeights The weight of each processed used in the split procedure.
	 * 		set to NULL to consider equal weights.
	 */
	void setForkCount(const int forkCount, const int* forkWeights = NULL);

	/**
	 * Defines the error during the execution of the
	 * AbstractAlignerParameters::processArgument method.
	 *
	 * @param error The string containing the argument error.
	 */
	void setLastError(const char* error);

	/**
	 * Prints the usage text in the command line.
	 * @param header The highlighted header of the help section that will contain
	 * 		the customized usage string.
	 * @param text The string containing the help text that will be shown to the user.
	 */
	static void printFormattedUsage(const char* header, const char* text);

	/**
	 * Process the arguments using the optarg library.
	 */
	static int callGetOpt(int argc, char** argv, option* arguments);

private:
	/** The last error */
	char lastError[500];

	/** The Id of this forked process */
	int forkId;

	/** Arguments passed to the getopt library */
	const option* arguments;
};

#endif /* ABSTRACTALIGNERPARAMETER_HPP_ */
