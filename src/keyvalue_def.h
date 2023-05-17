/* 
   Copyright (C) Guido Schade 2001
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef KEYVALUEDEF_H 
#define KEYVALUEDEF_H 

#include <string.h>
#include <string>

#include "global.h"

#include "list_def.h"
#include "listelement_def.h"
#include "listelement_inl.h"

class KeyValue : public ListElement<KeyValue>
{ 
    public:

    KeyValue(const SBYTE * skey, CONFTYPE type = TYPE_STR, UBYTE options = CONF_UNIQ, const SBYTE * sval = NULL);
    ~KeyValue();

    inline SBYTE * getKey(VOID);
    inline SBYTE * getVal(VOID);

    STRING * key;
    STRING * val;

    CONFTYPE vtype;
    UBYTE options;
};

#endif // KEYVALUEDEF_H
