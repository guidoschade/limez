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

#ifndef TIMERDEF_H 
#define TIMERDEF_H 

#include "global.h"

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>

class Timer
{ 
  public:

    Timer();
    ~Timer();

    // TODO make 'em inline
    SBYTE * uptimeString();
    SBYTE * getDate(VOID);
    SBYTE * getSMTPDate(VOID);
    SBYTE * getRDate(VOID);
    SBYTE * getUnixString(VOID);
    ULONG  getUnix(VOID);
    ULONG  uptime(VOID);
  
  private:

    struct   timeval ltime;
    struct   timezone lzone;
    struct   tm * ltm;
    ULONG    started;
    SBYTE  * timestring;
    SBYTE    rdate[MAX_SMTP_DATE_SIZE+1];
};

#endif // TIMERDEF_H
