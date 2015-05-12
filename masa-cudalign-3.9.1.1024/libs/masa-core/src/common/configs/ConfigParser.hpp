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

#ifndef CONFIGFILE_HPP_
#define CONFIGFILE_HPP_

#include <stdio.h>
#include <string>
#include <map>
using namespace std;

struct config_option_t;

typedef void (*config_parser_t)(const char* opt, config_option_t* option);



typedef struct config_option_t {
	const char* section;
	const char* name;
	void* pointer;
	string default_value_str;
	config_parser_t parser;
	const char* option;
	string original_value_str;
	string resolved_value_str;

	/*config_option_t() {
		section = NULL;
		name = NULL;
		pointer = NULL;
		default_value_str = "";
		parser = NULL;
		option = NULL;
		original_value_str = "";
		resolved_value_str = "";
	}*/
} config_option_t;

class ConfigParser {
public:
	ConfigParser(config_option_t* options);
	virtual ~ConfigParser();

	void parseLine(const char* line);
	void printFile(FILE* file, const bool resolve = false);

	static string resolve_env(string in);

	static void parse_int      (const char* value, config_option_t* option);
	static void parse_int_min  (const char* value, config_option_t* option);
	static void parse_int_max  (const char* value, config_option_t* option);
	static void parse_int_range(const char* value, config_option_t* option);
	static void parse_int_enum (const char* value, config_option_t* option);
	static void parse_longlong_size(const char* value, config_option_t* option);
	static void parse_bool     (const char* value, config_option_t* option);
	static void parse_path     (const char* value, config_option_t* option);

private:
	map<string, map<string, config_option_t*> > options;
	char section[512];

	void tokenize(const char* line, char* param, char* value);
	void parseValue(const char* section, const char* param, const char* value);
};

#endif /* CONFIGFILE_HPP_ */
