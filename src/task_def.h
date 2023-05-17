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

#ifndef TASKDEF_H 
#define TASKDEF_H 

#include "global.h"

#include "log_def.h"

#include "list_def.h"
#include "listelement_def.h"
#include "message_def.h"
#include "signalhandler_def.h"

#include "listelement_inl.h"

class Task : public ListElement<Task>
{ 
  public:

    Task();
    virtual ~Task();

    virtual BOOL start(VOID);
    virtual BOOL deny(VOID);

    inline JOB  getJob(VOID);
    inline SLONG getPid(VOID);
    inline SBYTE * getSPid(VOID);
    inline ULONG getStart(VOID);
    inline VOID setPid(SLONG tpid);
    inline VOID setStart(ULONG tstart);
    inline VOID setJob(JOB tjob);
    inline VOID addErr(VOID);

    // must be public (accessed from global signal handler)
    List<Task> tasks;
    SignalHandler * sigCHLD;

  protected:

    SignalHandler * sigINT, * sigHUP, * sigALRM, * sigPIPE;
    SignalHandler * sigUSR1, * sigUSR2;
    Message * currMsg;

    SBYTE spid[MAX_INT_STR_SIZE+1];
    ULONG errors;

  private:

    ULONG tstart, id;
    SLONG tpid;
    JOB tjob;
};

#endif // #ifndef TASKDEF_H
