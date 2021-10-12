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

#include "ConfigParser.hpp"
#include "../exceptions/IllegalArgumentException.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>
#include <sstream>
using namespace std;

#define DEBUG (0)

ConfigParser::ConfigParser(config_option_t* options) {
	config_option_t* ptr = options;
	while (ptr->name != NULL && ptr->section != NULL) {
		this->options[ptr->section][ptr->name] = ptr;
		if (DEBUG) fprintf(stderr, "v: %s:%s %p def:%s\n", ptr->section, ptr->name, this->options[ptr->section][ptr->name], ptr->default_value_str.c_str());

		if (ptr->pointer == NULL) {
			//throw IllegalArgumentException("ConfigParser cannot be created. Missing flag pointer.", ptr->name);
			fprintf(stderr, "ConfigParser cannot be created. Missing flag pointer. %s:%s\n", ptr->section, ptr->name);
			return;
		}
		ptr->resolved_value_str = ptr->default_value_str;

		ptr++;
	}

	strcpy(section, "[global]");
}

ConfigParser::~ConfigParser() {

}

string ConfigParser::resolve_env(string in) {
	wordexp_t p;
	char **w;
	int i;

	if (in.length() == 0) {
		throw IllegalArgumentException("Empty argument.");
	}

	int ret = wordexp(in.c_str(), &p, WRDE_NOCMD | WRDE_UNDEF);
	std::stringstream var;
	var << "Variable: " << in;
	if (ret == WRDE_BADVAL) {
		throw IllegalArgumentException("An undefined environment variable was referenced.", var.str().c_str());
	} else if (ret != 0) {
		throw IllegalArgumentException("Wrong environment variable expansion.", var.str().c_str());
	} else if (p.we_wordc == 0) {
		throw IllegalArgumentException("No wildcard ('*') expansion.", var.str().c_str());
	} else if (p.we_wordc > 1) {
		throw IllegalArgumentException("Ambiguous wildcard ('*') expansion.", var.str().c_str());
	}
	w = p.we_wordv;
	string out = w[0];
	wordfree(&p);

	return out;
}

void ConfigParser::printFile(FILE* file, bool resolve) {
	for(map<string, map<string, config_option_t*> >::iterator iter_sections = options.begin(); iter_sections != options.end(); ++iter_sections)	{
		const char* section = iter_sections->first.c_str();
		if (DEBUG) printf("\n%s\n", section);
		for(map<string, config_option_t*>::iterator iter = iter_sections->second.begin(); iter != iter_sections->second.end(); ++iter)	{
			const char* k = iter->first.c_str();
			const config_option_t* option = iter->second;
			fprintf(file, "%s\t%s\n", k, resolve ? option->resolved_value_str.c_str() : option->original_value_str.c_str());
		}
	}
}

void ConfigParser::tokenize(const char* _line, char* _param, char* _value) {
	char line[512];
	char* param;
	char* value;
	strcpy(line, _line);
	while (line[strlen(line) - 1] == '\n' || line[strlen(line) - 1] == '\r') {
		line[strlen(line) - 1] = '\0';
	}
	char* sharp = strchr(line, '#');
	if (sharp != NULL) {
		*sharp = '\0';
	}
	param = line;
	while (*param == '\t' || *param == ' ') {
		param++;
	}
	if (*param == '\0') {
		_param[0] = '\0';
		return;
	}
	value = param;
	while (*value != '\t' && *value != ' ' && *value != '\0') {
		value++;
	}
	if (*value != '\0') {
		*value = '\0'; // terminating the parameter name token.
		value++;
		while (*value == '\t' || *value == ' ') {
			value++;
		}
	}

	strcpy(_param, param);
	strcpy(_value, value);
	if (DEBUG) fprintf(stderr, "%s -> %s\n", param, value);
}

void ConfigParser::parseValue(const char* section, const char* param, const char* value) {
	if (options[section][param] == NULL) {
		char msg[128];
		sprintf(msg, "Unknown parameter `%s'.", param);
		throw IllegalArgumentException(msg);
	}
	config_option_t* option = options[section][param];
	if (*value == '\0') {
		char msg[128];
		sprintf(msg, "An argument must be supplied to the parameter: %s.\n",
				param);
		throw IllegalArgumentException(msg);
	} else {
		char *cpy_value = new char[strlen(value)+1];
		strcpy(cpy_value, value);
		option->original_value_str = cpy_value;
		option->resolved_value_str = cpy_value;

		option->parser(value, option);
	}
}

void ConfigParser::parseLine(const char* line) {
	char param[128];
	char value[128];

	tokenize(line, param, value);
	if (*param == '\0') {
		return;
	}
	if (*param == '[') {
		strcpy(section, param);
		if (options[section].size() == 0) {
			char msg[128];
			sprintf(msg, "Unknown section `%s'.", param);
			throw IllegalArgumentException(msg);
		}
		return;
	}
	parseValue(section, param, value);
}

void ConfigParser::parse_int (const char* value, config_option_t* option) {
	int* i = (int*)option->pointer;
	if ( sscanf ( value, "%d", i ) != 1 ) {
		char msg[128];
		sprintf(msg, "Value must be an integer");
		throw IllegalArgumentException(msg);
	}
}


void ConfigParser::parse_int_range (const char* value, config_option_t* option) {
	int* i = (int*)option->pointer;
	sscanf ( value, "%d", i );
	int min;
	int max;
	sscanf (option->option, "%d %d", &min, &max);
	if ( *i < min || *i > max ) {
		char msg[128];
		sprintf(msg, "Value must be in range %d..%d", min, max);
		throw IllegalArgumentException(msg);
	}
}

void ConfigParser::parse_int_min (const char* value, config_option_t* option) {
	int* i = (int*)option->pointer;
	sscanf ( value, "%d", i );
	int min;
	sscanf (option->option, "%d", &min);
	if ( *i < min ) {
		char msg[128];
		sprintf(msg, "Value must be greater or equal to %d", min);
		throw IllegalArgumentException(msg);
	}
}

void ConfigParser::parse_int_max (const char* value, config_option_t* option) {
	int* i = (int*)option->pointer;
	sscanf ( value, "%d", i );
	int max;
	sscanf (option->option, "%d", &max);
	if ( *i > max ) {
		char msg[128];
		sprintf(msg, "Value must be less or equal to %d", max);
		throw IllegalArgumentException(msg);
	}
}

void ConfigParser::parse_longlong_size (const char* value, config_option_t* option) {
	long long* ll = (long long*)option->pointer;
	if (strcmp(value, "none") == 0) {
		*ll = 0;
		return;
	}

	string str = value;
	char suffix = str[str.length()-1];
	str[str.length()-1] = 0;
	switch ( suffix ) {
	case 'K':
		*ll = ( long long ) ( atof ( str.c_str() ) *1024LL );
		break;
	case 'M':
		*ll = ( long long ) ( atof ( str.c_str() ) *1024*1024LL );
		break;
	case 'G':
		*ll = ( long long ) ( atof ( str.c_str() ) *1024*1024*1024LL );
		break;
	default:
		throw IllegalArgumentException("Wrong size suffix (use 'K', 'M' or 'G').");
	}
	if ( *ll == 0 ) {
		throw IllegalArgumentException("Wrong disk size limit.");
	}
	char tmp[256];
	sprintf(tmp, "%lld", *ll);
	option->resolved_value_str = tmp;
}

void ConfigParser::parse_int_enum(const char* value, config_option_t* option) {
	int* i = (int*)option->pointer;

	char option_cpy[256];
	strncpy(option_cpy, option->option, sizeof(option_cpy));
	option_cpy[sizeof(option_cpy)-1] = '\0';

	char* tok = strtok(option_cpy, ";");
	int id = 0;
	while (tok != NULL) {
		if (strcmp(tok, value) == 0) {
			*i = id;
			return;
		}
		id++;
		tok = strtok(NULL, ";");
	}

	char msg[128];
	sprintf(msg, "Option `%s' not recognized. Possible values are: %s", value, option->option);
	throw IllegalArgumentException(msg);
}

void ConfigParser::parse_bool (const char* value, config_option_t* option) {
	int* b = (int*)option->pointer;
	if (strcmp(value, "enabled") == 0 || strcmp(value, "true") == 0) {
		*b = 1;
		return;
	} else if (strcmp(value, "disabled") == 0 || strcmp(value, "false") == 0) {
		*b = 0;
		return;
	} else {
		char msg[128];
		sprintf(msg, "Value must be enabled/disable or true/false");
		throw IllegalArgumentException(msg);
	}
}

void ConfigParser::parse_path (const char* value, config_option_t* option) {
	string* path = (string*)option->pointer;
	*path = resolve_env(string(value));
	option->resolved_value_str = *path;
}
