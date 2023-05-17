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

#include "scheduler_def.h"
#include "log_def.h"
#include "config_def.h"
#include "connection_inl.h"

extern Log                log;
extern Config             config;
extern Timer              timer;
extern Scheduler          scheduler;
extern List<MailingList>  lists;

extern Task * task;

static VOID scheduler_signal(INT sig);
static VOID scheduler_sigchild(INT sig);
static VOID scheduler_sigalarm(INT sig);

Scheduler::Scheduler()
:sigCHLD(NULL),sigINT(NULL),sigHUP(NULL),sigUSR1(NULL),sigUSR2(NULL),sigALRM(NULL),sigPIPE(NULL)
{
  initialized = FALSE;
  task = NULL;
  reloads = 0;
  connections = 0;
}

Scheduler::~Scheduler()
{
}

// initialize scheduler & lists (called at start and by SIGHUP)
BOOL Scheduler::init(VOID)
{
  MailingList  * currList = NULL;
  SBYTE * clist = NULL, * c = config.getValue(KEY_SERVER_CONFIG);
  ULONG lnum = 0;
  SBYTE  conf[MAX_VAL_SIZE+1];

  DEBUGGER("Scheduler::init()");
  ASSERT(c);
  strncpy(conf, c, MAX_VAL_SIZE);

  if (!initialized)
  {
    // installing signal handlers
    sigINT  = new SignalHandler(SIGINT, scheduler_signal);
    sigHUP  = new SignalHandler(SIGHUP, scheduler_signal);
    sigPIPE = new SignalHandler(SIGPIPE, scheduler_signal);
    sigUSR1 = new SignalHandler(SIGUSR1, scheduler_signal);
    sigUSR2 = new SignalHandler(SIGUSR2, scheduler_signal);
    sigCHLD = new SignalHandler(SIGCHLD, scheduler_sigchild);
    sigALRM = new SignalHandler(SIGALRM, scheduler_sigalarm);
  }
  else
  {
    reloads ++;

    // prepare reread of config
    config.clear();
    config.setDefaults();
  }

  // reading limez main config / create defaults
  if (config.read(conf) == FALSE)
  {
    log.add(1,"info: empty mainconfig (%s) detected, will be filled with default values", conf);
    config.write(conf);
  }

  // exit scheduler if check failed
  if (config.check() == FALSE)
    exit(1);

  // displaying config if requested via console
  if (!initialized && config.getBoolValue(KEY_DEBUG_TEST_ONLY))
    config.dump();

  log.setDebug(config.getIntValue(KEY_DEBUG_LEVEL));
  log.setFile(config.getValue(KEY_LOGFILE));

  // drop all mailinglists
  if (initialized)
    lists.clear();

  // reading configs of all file mailinglists
  lnum = 0;
  while ((clist = config.getNext(++lnum, KEY_MAILINGLIST)))
  {
    log.add(4,"info: reading list '%s'", clist);
    currList = new MailingList();
    ASSERT(currList);

    currList->initConfig(clist, FALSE);
    lists.add(currList);
  }

  // reading configs of all db mailinglists
  lnum = 0;
  while ((clist = config.getNext(++lnum, KEY_MAILINGLIST_DB)))
  {
    log.add(4,"info: reading db list '%s'", clist);
    currList = new MailingList();
    ASSERT(currList);

    currList->initConfig(clist, TRUE);
    lists.add(currList);
  }

  // fork into background if requested
  if ((!initialized) && config.getBoolValue(KEY_SERVER_DAEMONIZE))
  {
    if (fork() != 0)
    {
      // parent terminates
      sleep(1);
      log.add(0,"\n");
      close(STDIN_FILENO);
      close(STDOUT_FILENO);
      close(STDERR_FILENO);
      exit(0);
    }
    else
      log.setPid(getpid());
  }

  initialized = TRUE;
  return(TRUE);
}

// child (task) status + care about spooling
static VOID scheduler_sigalarm(INT sig)
{
  Task * currTask = NULL, * nextTask = NULL;
  UBYTE num;
  ULONG runtime, interval;
  SLONG pid;
  pid_t npid;

  DEBUGGER("scheduler_sigalarm()");

  // blocking SIGCHLD
  scheduler.sigCHLD->suspend();

  // setting alarm again
  if ((interval = config.getIntValue(KEY_SERVER_SPOOL_INTERVAL)))
    alarm(interval);

  // check runtime of all tasks
  if ((num = scheduler.tasks.getCount()))
  {
    log.add(4,"info: %d tasks running in scheduler", num);

    currTask = scheduler.tasks.getFirst();
    while(currTask)
    {
      pid = currTask->getPid();
      runtime = timer.getUnix() - currTask->getStart(); 
      log.add(4,"info: task (%ld) is running %ld secs in scheduler", pid, runtime);

      nextTask = scheduler.tasks.getNext(currTask);
      if (runtime > MAX_TASK_TIME)
      {
        log.add(3,"info: scheduler is terminating task (%ld)", pid);

        scheduler.tasks.del(currTask);
        delete currTask;

        if (kill(pid, SIGTERM) == -1)
          log.add(1,"error: sending signal SIGTERM to pid %dl failed (%s)", pid, strerror(errno));
      } 
      currTask = nextTask;
    }
  }
  else
    log.add(5,"info: no tasks running in scheduler");

  // creating new task for spoolrun  
  task = new Manager();
  ASSERT(task);

  task->setStart(timer.getUnix());
  task->setJob(MODE_TRANSMIT);

  switch((npid = fork()))
  {
    case(-1):
    {
      log.add(1,"error: critical, fork() failed (%s)", strerror(errno));
      delete currTask;
      scheduler.sigCHLD->release();
      return;
      break;
    }

    case(0):
    {
      // task (child) execution and exit
      task->setPid((npid = getpid()));
      scheduler.tasks.add(task);
      scheduler.sigCHLD->release();

      log.setPid(npid);
      task->start();
      exit(0);
      break;
    }

    default:
    {
      // scheduler (parent) stuff
      task->setPid(npid);
      scheduler.tasks.add(task);
      scheduler.sigCHLD->release();
      return;
      break;
    }
  }
}

// global scheduler exception handler
static VOID scheduler_signal(INT sig)
{
  INT level;

  switch(sig)
  {
    case(SIGHUP):
    {
      log.add(2,"info: rereading configs due to signal ...");
      scheduler.init();
      break;
    }

    case(SIGPIPE):
    {
      log.add(1,"info: received SIGPIPE, scheduler shouldn't, bad thing ...");
      break;
    }

    case(SIGINT):
    {
      log.add(1,"info: stop signal received, limez is shutting down");
      scheduler.quit();
      exit(0);
      break;
    }

    case(SIGUSR1):
    {
      level = log.getDebug() + 1;
      if (level < DEBUG_MAX)
      {
        log.add(2,"info: debug level increased to %d by signal", level);
        log.setDebug(level);
      }
      else
        log.add(2,"error: already at highest debug-level (%ld)", DEBUG_MAX);
      break; 
    }

    case(SIGUSR2):
    {
      level = log.getDebug() - 1;
      if (level >= 0)
      {
        log.add(2,"info: debug level decreased to %d by signal", level);
        log.setDebug(level);
      }
      else
        log.add(2,"error: already at lowest debug-level (0)");
      break;
    }

    default:
      log.add(2,"error: unknown signal %d received, ignored", sig);
      break;
  }
  return;
}

// end scheduler
VOID Scheduler::quit()
{
  if (conn.getStatus())
    conn.sclose();

  // remove semaphore TODO HELP XXX
  exit(0);
}

// start listening
VOID Scheduler::run()
{
  ULONG   count, spoolcount = 0;
  SBYTE * s = NULL, * spoolonly = config.getValue(KEY_SERVER_SPOOL_ONLY);
  ULONG   interval = config.getIntValue(KEY_SERVER_SPOOL_INTERVAL);
  pid_t   pid;

  DEBUGGER("Scheduler::run()");

  if (config.getBoolValue(KEY_DEBUG_TEST_ONLY) == TRUE)
  {
    log.add(0,"\nConfig okay, %ld list(s) successfully checked, to reread configuration", lists.getCount());
    log.add(0,"of the running listserver, please send a HUP-Signal to the parent-process");
    log.add(0,"(kill -HUP pid), exiting now.");
    exit(0);
  }
  else
    log.add(1,"info: limez version %s started", VERSION);

  // init semaphore TODO HELP XXX

  // do not listen on socket, spool only
  if (spoolonly)
  {
    if ((spoolcount = atol(spoolonly)))
      log.add(1,"info: only taking care of spool area %ld times every %ld secs, not listening on socket",
                spoolcount, interval);
    else
      log.add(1,"info: only taking care of spool area every %ld secs, not listening on socket", interval);
  }
  else
  {
    // preparing connection
    if (conn.prepare() == FALSE)
    {
      log.add(1,"error: failed to listen to requested ip/port, exiting.");
      return;
    }
  }

  // setting chroot
  if ((s = config.getValue(KEY_SERVER_ROOT)))
  {
    log.add(4,"info: applying chroot environment to '%s'", s);
    if (chroot(s) == -1)
      log.add(1,"warning: chroot call (%s) failed (%s)", s, strerror(errno));
  }

  // setting group id if set
  if ((s = config.getValue(KEY_SERVER_GID)))
  {
    log.add(4,"info: setting gid and egid to '%s'", s);
    if (setgid(atoi(s))<0)
      log.add(1,"warning: setting of groupid '%s' failed (%s)", s, strerror(errno));

    if (setegid(atoi(s))<0)
      log.add(1,"warning: setting of effective groupid '%s' failed (%s)", s, strerror(errno));
  }

  // setting user id if set
  if ((s = config.getValue(KEY_SERVER_UID)))
  {
    log.add(4,"info: setting uid and euid to '%s'", s);
    if (setuid(atoi(s))<0)
      log.add(1,"warning: setting of userid '%s' failed (%s)", s, strerror(errno));

    if (seteuid(atoi(s))<0)
      log.add(1,"warning: setting of effective userid '%s' failed (%s)", s, strerror(errno));
  }

  // test user-id
  if (getuid() == 0)
  {
    log.add(1,"warning: limez is running with super-user privileges, to avoid this");
    log.add(1,"message please set 'server_uid' and 'server_gid' in limez main config.");
  }

  // spooling timer
  if (interval)
    alarm(interval);

  // spool only and exit
  if (spoolonly && interval)
  {
    UWORD y = 0;

    // HELP TODO XXX should not mix sleep and alarm
    while(y++ < spoolcount || !spoolcount)
      sleep(interval);
    quit();
  }

  // main receive loop
  while(TRUE)
  {
    // accept
    if (conn.awaiting() == FALSE)
      break;

    connections ++;

    // prepare receiver
    task = new Receiver(&conn);
    ASSERT(task);

    task->setJob(MODE_RECEIVE);
    task->setStart(timer.getUnix());

    sigCHLD->suspend();
    switch((pid = fork()))
    {
      case(-1):
      {
        log.add(1,"error: critical, fork() failed, absolutely bad (%s)", strerror(errno));
        exit(1);
        break;
      }

      case(0):
      {
        // task (child) execution and exit
        count = tasks.getCount();
        log.add(6,"info: %ld tasks running in scheduler (before adding)", count);
        tasks.add(task);

        sigCHLD->release();

        pid = getpid();
        log.setPid(pid);
        task->setPid(pid);

        if (count > config.getIntValue(KEY_SERVER_MAX_PROCS))
          task->deny();
        else
          task->start();

        exit(0);
        break;
      }

      default:
      {
        // scheduler (parent) stuff
        task->setPid(pid);
        tasks.add(task);

        count = tasks.getCount();
        log.add(6,"info: %ld tasks running in scheduler (after adding)", count);

        sigCHLD->release();

        // close connection socket without shutdown
        conn.cclose(FALSE);
        log.add(4,"info: scheduler connection closed, waiting for next");
        break;
      }
    }
  }
  quit();
}

// task death (global signal handler function)
static VOID scheduler_sigchild(INT sig)
{
  INT    status, pid, found = FALSE;
  Task * currTask = NULL, * nextTask = NULL;

  while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
  {
    if (status & 0xff)
      log.add(1,"error: task (%ld) has been killed with signal %d, pretty bad", pid, status & 0xff);
    else
      log.add(4,"info: task (%ld) exited normally", pid);

    currTask = scheduler.tasks.getFirst();
    while(currTask)
    {
      nextTask = scheduler.tasks.getNext(currTask);
      if (pid == currTask->getPid())
      {
        found = TRUE;
        scheduler.tasks.del(currTask);
        delete currTask;
      }
      currTask = nextTask;
    }

    if (found == FALSE)
      log.add(1,"error: scheduler got SIGCHLD from unknown process, pretty bad (pid %ld)", pid);
  }
  return;
}
