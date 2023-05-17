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

#ifndef TASK_H
#define TASK_H

#include "task_def.h"
#include "log_def.h"

extern Log log;

inline SLONG Task::getPid(VOID)
{
  DEBUGGER("Task::getPid()");
  return(tpid);
}

inline SBYTE * Task::getSPid(VOID)
{
  DEBUGGER("Task::getSPid()");
  return((SBYTE *) &spid);
}

inline ULONG Task::getStart(VOID)
{
  DEBUGGER("Task::getStart()");
  return(tstart);
}

inline JOB Task::getJob(VOID)
{
  DEBUGGER("Task::getJob()");
  return(tjob);
}

inline VOID Task::setPid(SLONG tpid)
{
  DEBUGGER("Task::setPid()");
  this->tpid = tpid;
  snprintf(spid, MAX_INT_STR_SIZE, "%ld", tpid);
}

inline VOID Task::setStart(ULONG tstart)
{
  DEBUGGER("Task::setStart()");
  this->tstart = tstart;
}

inline VOID Task::setJob(JOB tjob)
{
  DEBUGGER("Task::setJob()");
  this->tjob = tjob;
}

inline VOID Task::addErr(VOID)
{
  DEBUGGER("Task::addErr()");
  errors++;
}
#endif