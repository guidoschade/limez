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

#ifndef LOGDEF_H 
#define LOGDEF_H 

#include "global.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "storage_def.h"
#include "filestorage_def.h"

class Log
{ 
  public:

    Log();
    ~Log();

    VOID add(ULONG lev, const SBYTE * ...);

    inline VOID setFile(const SBYTE * filename);
    inline VOID setPid(UWORD pid);
    inline VOID setDebug(UBYTE debuglevel);
    inline UBYTE getDebug(VOID);

  private:

    STRING filename;
    UBYTE debuglevel;
    UWORD pid;
    SBYTE buf[LOG_BUFSIZE+1];
};

#endif // LOGDEF_H
