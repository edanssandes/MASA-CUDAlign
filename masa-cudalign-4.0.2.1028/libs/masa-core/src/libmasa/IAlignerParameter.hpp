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

#ifndef IALIGNERPARAMETER_HPP_
#define IALIGNERPARAMETER_HPP_


/*
 * Used in the forkId parameter to indicates that the current process is not
 * a forked process
 */
#define NOT_FORKED_INSTANCE	(-1)

#define ARGUMENT_OK					(0)
#define ARGUMENT_ERROR				(-1)
#define ARGUMENT_ERROR_NOT_FOUND	(-2)
#define ARGUMENT_ERROR_NO_OPTION	(-3)

/**
 * The customized parameters of a MASA Extension, used for receiving,
 * manipulating and customizing command line parameters.
 *
 * Each MASA Extension must have its own subclass of the
 * IAlignerParameters class. The subclass may customize
 * the command line parameters using the <a href="http://www.gnu.org/software/libc/manual/html_node/Getopt.html">GNU getopt C library</a>.
 * This is done by calling
 * the AbstractAlignerParameters::callGetOpt method with the option structure
 * of the getopt library. The IAlignerParameters::processArgument() pure
 * virtual method will be invoked whenever MASA encounters one of the
 * customized parameters defined with the setArguments method. Using this
 * method, all the parameters may be set as an attribute of the subclass.
 *
 * The aligner must set default values for each parameters because the
 * IAlignerParameters::processArgument()
 * methods may never be called if there is no argument.
 *
 * The Aligner class must hold one instance of the IAlignerParameters
 * class and it must return this instance in the IAlign::getParameters() method.
 *
 * One command line that the IAlignerParameters must manipulate is the
 * <tt>--fork</tt> parameter, since MASA use this parameter to fork as many
 * processes as the architecture supports. If MASA forks many processes,
 * the IAlignerParameters::getForkId() method will return the
 * identifier of this forked process (from 0 to n-1), or NOT_FORKED_INSTANCE
 * if there was no forked process. Use this
 * Id to customize the execution uniquely to your architecture.
 *
 * The AbstractAlignerParameters class implements the basic operations of the
 * IAlignerParameter. Extend this class to implement Aligner specific parameters.
 * You can create a hierarchy of parameter classes, but always call the overwritten
 * methods in order to parse the parameters of the super classes.
 *
 * @see AbstractAligner, AbstractAlignerParameters
 * @see <a href="http://www.gnu.org/software/libc/manual/html_node/Getopt.html">GNU getopt C library</a> (External link)
 */
class IAlignerParameters {


public:

	/**
	 * Prints the usage text in the command line. Use the
	 * AbstractAlignerParameters::printUsage method to print
	 * with a pretty bash format.
	 */
	virtual void printUsage() const = 0;

	/**
	 * Processes each customized command line option.
	 *
	 * For instance, suppose that the customized parameter was "--arg=OPT". The
	 * we will have this methos will be invoked with the following parameters:
	 * <ul>
	 *  <li>argument_str: the code associated with "--arg" in the option structure;
	 *  <li>argument_str: the "--arg=OPT" string;
	 *  <li>argument_option: the "OPT" string.
	 * </ul>
	 * Inside this method, the subclass should set the "arg" variable to OPT.
	 *
	 * @param argument_code The code of the argument associated in the
	 * 		AbstractAlignerParameters::setArguments method.
	 * @param argument_str The argument string with its option.
	 * @param argument_option The option passed to the argument. NULL if there
	 * 		is not any argument.
	 * @return ARGUMENT_OK (0) if succeeded, non-zero otherwise (ARGUMENT_ERROR,
	 * 		ARGUMENT_ERROR_NOT_FOUND or ARGUMENT_ERROR_NO_OPTION). If there is
	 * 		an error, set the error string using the
	 * 		AbstractAlignerParameters::setLastError	function.
	 */
	virtual int processArgument(int argc, char** argv) = 0;

	/**
	 * Returns the last error defined by the AbstractAlignerParameters::setLastError
	 * method.
	 * @return the last error string.
	 */
	virtual const char* getLastError() const = 0;

	/**
	 * Returns the Id of the forked process.
	 *
	 * @return the Id of the forked process, from 0 to n-1 (where n is the
	 * number of forked process), or NOT_FORKED_INSTANCE (-1) if no process
	 * was forked.
	 */
	virtual int getForkId() const = 0;

	/**
	 * Defines the Id of the forked process.
	 * @param forkId the unique Id of this forked process.
	 */
	virtual void setForkId(int forkId) = 0;


protected:
/* protected constructors avoid the direct creation/deletion of this interface */
		~IAlignerParameters() {};
		IAlignerParameters() {};

};


#endif /* IALIGNERPARAMETER_HPP_ */
