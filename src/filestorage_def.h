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

#ifndef FILESTORAGEDEF_H 
#define FILESTORAGEDEF_H

#include "global.h"
#include "storage_def.h"

#include <stdio.h>

#include "datablock_def.h"

class FileStorage : public Storage
{ 
  public:

    FileStorage(STORECLASS sclass, FILEMODE fmode);
    ~FileStorage();
    
    BOOL open(const SBYTE * fname);
    BOOL close(VOID);
    BOOL add(DataBlock * block);
    BOOL get(DataBlock * block, ULONG start = 1, ULONG count = 0);
    BOOL remove(DataBlock * block);
    BOOL find(DataBlock * block);
    BOOL change(DataBlock * block);
    BOOL destroy(SBYTE * nam);
    BOOL reName(const SBYTE * newname);
    BOOL clone(const SBYTE * newname);
    ULONG getCount(VOID);

  private:
    
    ULONG line;
    FILE * file;
    SBYTE buf[MAX_LINE_SIZE];
};

#endif // #ifndef FILESTORAGEDEF_H
