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

#include "keyvalue_def.h"

KeyValue::~KeyValue()
{
  delete key;

  if (val)
    delete val;
}

KeyValue::KeyValue(const SBYTE * skey, CONFTYPE type = TYPE_STR, UBYTE options = CONF_UNIQ, const SBYTE * sval = NULL)
{
  ASSERT(skey);

  key = new STRING;
  *key = skey;

  if (sval != NULL)
  {
    val = new STRING;
    *val = sval;
  }
  else
    val = NULL;

  this->vtype = type;
  this->options = options;
}
