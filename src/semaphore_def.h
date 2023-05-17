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

#ifndef SEMAPHOREDEF_H 
#define SEMAPHOREDEF_H 

#include "global.h"

#include <sys/types.h>

#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>

#include "log_def.h"

class Semaphore
{ 
  public:

     Semaphore();
     ~Semaphore();

     inline BOOL init(VOID);
    inline BOOL lock(VOID);
    inline BOOL unlock(VOID);
    inline BOOL remove(VOID);

  private:

    INT    semaphoreId;
    key_t  semaphoreKey;
    struct sembuf * semaphoreOps;

};

#endif // SEMAPHOREDEF_H
