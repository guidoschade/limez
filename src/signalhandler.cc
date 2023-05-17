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

#include "signalhandler_def.h"
#include <strings.h>

extern Log log;

SignalHandler::SignalHandler(INT s, VOID (*handler)(INT))
{
  ASSERT(s > 0);
  active = FALSE;
  sig = s;
  install(handler);
}

SignalHandler::~SignalHandler()
{
}

// installing the given handler for the given signal
BOOL SignalHandler::install(VOID (*handler)(INT))
{
  DEBUGGER("SignalHandler::install()");
  ASSERT(!active);

  struct sigaction n, o;

  bzero(&n, sizeof n);
  n.sa_sigaction = (VOID(*)(INT, siginfo_t *, VOID *)) handler;
  n.sa_flags = SA_RESTART|SA_SIGINFO;
  n.sa_handler = handler;

  if (sigaction(sig, &n, &o) < 0)
  {
    log.add(1,"error: install of signalhandler (%d) failed", sig);
    active = FALSE;
  }
  else
    active = TRUE;

  return(active);
}

// suspend signal
BOOL SignalHandler::suspend(VOID)
{
  DEBUGGER("SignalHandler::suspend()");

  //ASSERT(active);
  if (!active)
    log.add(1,"error: ASSERTION active FAILED !!!!!............don't know why yet (solaris)");

  sigset_t sset, oset;

  sigemptyset(&sset);
  sigaddset(&sset, sig);

  if (sigprocmask(SIG_BLOCK, &sset, &oset) < 0)
  {
    log.add(1,"error: suspend of signal (%d) failed", sig);
    return(FALSE);
  }
  else
  {
    active = FALSE;
    return(TRUE);
  }
}

// release suspended signal
BOOL SignalHandler::release(VOID)
{
  DEBUGGER("SignalHandler::release()");
  ASSERT(!active);

  sigset_t sset, oset;

  sigemptyset(&sset);
  sigaddset(&sset, sig);

  if (sigprocmask(SIG_UNBLOCK, &sset, &oset) < 0)
  {
    log.add(1,"error: release of signal (%d) failed", sig);
    return(FALSE);
  }
  else
  {
    active = TRUE;
    return(TRUE);
  }
}
