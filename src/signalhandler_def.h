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

#ifndef SIGNALHANDLERDEF_H 
#define SIGNALHANDLERDEF_H

#include "global.h"

#include <signal.h>
#include <errno.h>

#include "log_def.h"

class SignalHandler
{ 
  public:

     SignalHandler(INT sig, VOID (*handler)(INT));
     ~SignalHandler();

    BOOL  install(VOID (*handler)(INT));
    BOOL  suspend(VOID);
    BOOL  release(VOID);

  private:

    INT sig;
    BOOL active;

};

#endif // SIGNALHANDLERDEF_H
