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

#ifndef STORAGEDEF_H 
#define STORAGEDEF_H

#include "global.h"

#include "datablock_def.h"

class Storage
{ 
  public:

    Storage();
    virtual ~Storage();

    virtual BOOL open(const SBYTE * name);
    virtual BOOL close(VOID);
    virtual BOOL add(DataBlock * block);
    virtual BOOL get(DataBlock * block, ULONG start = 1, ULONG count = 0);
    virtual BOOL remove(DataBlock * block);
    virtual BOOL find(DataBlock * block);
    virtual BOOL destroy(SBYTE * nam);
    virtual BOOL reName(const SBYTE * newname);
    virtual BOOL clone(const SBYTE * newname);
    virtual BOOL change(DataBlock * block);
    virtual ULONG getCount(VOID);

  protected:
  
    SBYTE name[MAX_VAL_SIZE+1];
    BOOL opened;
    FILEMODE fmode;
    STORECLASS sclass;
};

#endif // #ifndef STORAGEDEF_H
