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

#include "task_def.h"
#include "task_inl.h"

extern Log log;

Task::Task()
:sigCHLD(NULL),sigINT(NULL),sigHUP(NULL),sigALRM(NULL),sigPIPE(NULL),sigUSR1(NULL),sigUSR2(NULL)
{
  currMsg = NULL;
  id = 0;
  errors = 0;
}

Task::~Task()
{
}

BOOL Task::start(VOID)
{
  DEBUGGER("Task::start()");
  return(TRUE);
}

BOOL Task::deny(VOID)
{
  DEBUGGER("Task::deny()");
  return(TRUE);
}