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

#ifndef CONFIGDEF_H 
#define CONFIGDEF_H 

#include "global.h"

#include <stdio.h>

#include "list_def.h"
#include "keyvalue_def.h"
#include "filestorage_def.h"
#include "connection_def.h"
#include "dbstorage_def.h"

class Connection;

class Config
{   
  public:

    Config(SBYTE * name);
    ~Config();
  
    BOOL    delValue(const SBYTE * key, SBYTE * val = NULL);
    BOOL    addValue(const SBYTE * key, const SBYTE * value = NULL, CONFOPT opt = CONF_NONE);

    SBYTE * getValue(const SBYTE * key);
    SBYTE * getNext(ULONG cnt, SBYTE * key);
    ULONG   getIntValue(const SBYTE * key);
    BOOL    getBoolValue(const SBYTE * key);

    VOID    clear();
    VOID    dump(VOID);
    VOID    dump(Connection * conn);
    VOID    dumpKeys(Connection * conn);
    BOOL    write(const SBYTE * n);
    BOOL    read(const SBYTE * n, const BOOL tst = FALSE);

    VOID     setName(SBYTE * n);
    VOID    initMainKeys(VOID);
    VOID    initListKeys(VOID);
    VOID    initMsgKeys(VOID);
    VOID    initLockKeys(VOID);
    VOID    setDefaults(VOID);
    BOOL    check(VOID);

    BOOL    remove(SBYTE * n);

  private:

    STRING  name;

    List<KeyValue> values;
    List<KeyValue> keys;
};

#endif // CONFIGDEF_H
