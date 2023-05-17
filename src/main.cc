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

#include <stdio.h>
#include <stdlib.h>

#include "global.h"

#include "scheduler_def.h"
#include "log_def.h"
#include "list_def.h"
#include "timer_def.h"
#include "config_def.h"
#include "resolver_def.h"
#include "mailinglist_def.h"
#include "generator_def.h"

// global objects & stuff
Log       log;
Timer     timer;
Config    config("limez");
Scheduler scheduler;
Resolver  dns;
Generator creator;

List<MailingList>  lists;
Task *  task = NULL;

extern VOID getOptions(INT argc, SBYTE ** argv);

// main function
INT main(INT argc, SBYTE ** argv)
{
  // prepare log (default values)
  log.setFile("limez.log");
  log.setDebug(3);
  log.setPid(getpid());

  // parse command line arguments
  getOptions(argc, argv);

  // start scheduler and exit
  if (scheduler.init() == TRUE)
    scheduler.run();

  exit(0);
}
