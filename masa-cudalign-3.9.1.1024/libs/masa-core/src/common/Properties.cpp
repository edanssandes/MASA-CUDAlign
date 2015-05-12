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

#include <stdlib.h>

#include "Properties.hpp"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

Properties::Properties() {

}

Properties::Properties(const Properties& orig) {
}

Properties::~Properties() {
}

int Properties::initialize(string filename) {
    FILE* f = fopen(filename.c_str(), "r");
    if (f == NULL) {
        return 0; // TODO lancar excecao
    }
    char str[500];
    while (fgets(str, sizeof(str), f) != NULL) {
        string line(str);
        line.resize(line.size()-1); // Remove white space
        int pos = line.find('=');
        if (pos > 0) {
            string key = line.substr(0, pos);
            string value = line.substr(pos+1, string::npos);
            printf("%s  =  %s\n", key.c_str(), value.c_str());
            propertyMap[key] = value;
        }
    }
    fclose(f);
    return 1;
}

string Properties::get_property(string key) {
    return propertyMap[key];
}

int Properties::get_property_int(string key) {
    return atoi(propertyMap[key].c_str());
}
