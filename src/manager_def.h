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

#ifndef MANAGERDEF_H
#define MANAGERDEF_H

#include <sys/types.h>
#include <dirent.h>

#include <sys/types.h>  // waitpid()
#include <sys/wait.h>   // waitpid()

#include "global.h"

#include "log_def.h"
#include "timer_def.h"
#include "task_def.h"

class Manager : public Task
{ 
  public:

    Manager();
    ~Manager();

    BOOL start(VOID);
    BOOL broadcast(Message * msg);
    BOOL process(Message * msg);
    Message * getNext(SBYTE * dname);
};

#endif
