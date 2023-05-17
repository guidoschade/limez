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

#ifndef SCHEDULERDEF_H 
#define SCHEDULERDEF_H

#include <sys/types.h>  // waitpid()
#include <sys/wait.h>   // waitpid()

#include "global.h"

#include "list_def.h"
#include "task_def.h"
#include "connection_def.h"
#include "signalhandler_def.h"
#include "transmitter_def.h"
#include "receiver_def.h"
#include "manager_def.h"

#include "list_inl.h"
#include "log_inl.h"

class Task;

class Scheduler
{ 
  public:

    Scheduler();
    ~Scheduler();

    BOOL init(VOID);
    VOID run(VOID);
    VOID quit(VOID);

    inline ULONG getReloads(VOID);
    inline ULONG getTasks(VOID);
    inline ULONG getConns(VOID);

    // must be public (accessed from global signal handler)
    List<Task> tasks;
    SignalHandler * sigCHLD;

  private:

    SignalHandler * sigINT, * sigHUP, * sigUSR1, * sigUSR2, * sigALRM, * sigPIPE;

    ULONG reloads, connections;
    BOOL initialized;
    Connection conn;
};

#endif // #ifndef SCHEDULERDEF_H
