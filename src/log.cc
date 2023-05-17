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

#include "log_def.h"
#include "timer_def.h"
#include <fstream>
#include <iostream>

extern Timer timer;

Log::Log()
{
  debuglevel = 0;
  pid = getpid();
}

Log::~Log()
{
}

// write new entry to logfile/stdout
VOID Log::add(ULONG lev, const SBYTE *fmt, ...)
{ 
  SBYTE * p = (SBYTE *)&buf, * x = NULL;
  va_list list;

  ASSERT(lev <= DEBUG_MAX && lev >= 0);
  ASSERT(fmt);
  ASSERT(pid);
    
  // not the right level ?
  if (lev > debuglevel)
    return;

  // extract parameters into string
  va_start(list, fmt);
  if (vsnprintf(p, LOG_BUFSIZE, fmt, list) == -1)
    cout << "warning: log buffer truncated" << endl;
  va_end(list);

  // remove return if in string
  x = p+(strlen(p)-1);
  if (*x == '\n')
    *x = '\0';

  // if level 1 also writes to stdout
  if (lev < 2 || filename.empty())
    cout << p << endl;
   
  // loglevel 0 only writes to stdout   
  if (lev && !filename.empty())
  {
    ofstream out(filename.c_str(), ios::app);
    if (!out)
      cout << "error: can't write to logfile: " << filename << endl;
    else
    {
      out << timer.getDate() << "-[" << pid << "]-" << lev << " " << p << endl;
      out.close();
    }
  }
}
