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

#ifndef MASANETCLI_HPP_
#define MASANETCLI_HPP_


#include "MasaNet.hpp"


#define CMD_OK		(0)
#define CMD_ERROR	(1)
#define CMD_ABORT	(-1)

typedef int (*command_f) (char *arg);

typedef struct {
  char *name;			/* User printable name of the function. */
  command_f func;			/* Function to call to do the job. */
  char *doc;			/* Documentation for this function.  */
} command_t;

class MasaNetCLI {
public:
	static void initialize();
	static void openConsole();

	static int cmd_connect(char* arg);
private:
	static MasaNet* masaNet;
	static command_t commands[];

	static char * command_generator(const char *text, int state);
	static char *strip_whitespaces(char * string);
	static command_t * find_command(char *name);
	static int execute_line(char *line);
	static char** console_completion(const char *text, int start, int end);

	static void printTopology(int type);

	static int cmd_help(char* arg);
	static int cmd_status(char* arg);
	static int cmd_quit(char* arg);
	static int cmd_show_connected(char* arg);
	static int cmd_show_connected_remote(char* arg);
	static int cmd_show_discovered(char* arg);
	static int cmd_show_ring(char* arg);
	static int cmd_create_ring(char* arg);
	static int cmd_test_ring(char* arg);
};

#endif /* MASANETCLI_HPP_ */
