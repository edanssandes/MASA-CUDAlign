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

#ifndef _PROPERTIES_HPP
#define	_PROPERTIES_HPP

#include <string>
#include <map>
using namespace std;

class Properties {
public:
    Properties();
    Properties(const Properties& orig);
    virtual ~Properties();

    int initialize(string filename);
    string get_property(string key);
    int get_property_int(string key);
private:
    map<string, string> propertyMap;
};

#endif	/* _PROPERTIES_HPP */

