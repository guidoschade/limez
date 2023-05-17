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

#include "message_def.h"
#include "message_inl.h"
#include "log_def.h"
#include "timer_def.h"
#include "config_def.h"
#include "datablock_inl.h"
#include "task_def.h"
#include "generator_def.h"

extern Log log;
extern Timer timer;
extern Config config;
extern List<MailingList> lists;
extern Task * task;
extern Generator creator;
extern MailingList * getList(const SBYTE * listname, const SBYTE * listdomain = NULL);

Message::Message()
{
  sender = currAdd = NULL;
  currLine = NULL;
  mconf = NULL;
  name = "";
  quiet = FALSE;
  ml = NULL;
  tries = 0;
}

Message::~Message()
{
  // drop configuration
  if (mconf)
  {
    delete mconf;
    mconf = NULL;
  }

  lines.clear();
  recipients.clear();
}

// lock current message
BOOL Message::lock(VOID)
{
  SBYTE * spool_dir = config.getValue(KEY_SERVER_SPOOL_DIR);
  Config * lock = NULL;
  STRING lockfile;

  DEBUGGER("Message::lock()");
  ASSERT(name.size());
  ASSERT(task);
  ASSERT(spool_dir);

  lock = new Config("lock");
  ASSERT(lock);
  lock->initLockKeys();
  lock->clear();
  lock->setDefaults();
  lock->addValue(KEY_LOCK_PID, task->getSPid());
  lock->addValue(KEY_LOCK_DATE, timer.getUnixString());
  lockfile = string(spool_dir) + name + FILE_LOCK_EXT;
  lock->write(lockfile.c_str());
  delete lock;

  return(TRUE);
}

// unlock (remove lock - file)
BOOL Message::unlock(const SBYTE * lnam = NULL)
{
  SBYTE * spool_dir = config.getValue(KEY_SERVER_SPOOL_DIR);
  STRING lockfile;

  DEBUGGER("Message::unlock()");
  ASSERT(name.size());
  ASSERT(spool_dir);

  if (lnam)
    lockfile = lnam;
  else
    lockfile = string(spool_dir) + name + FILE_LOCK_EXT;

  if (unlink(lockfile.c_str()) != 0)
  {
    log.add(1,"error: remove of lock-file %s failed ...", lockfile.c_str());
    return(FALSE);
  }
  return(TRUE);
}

// return TRUE if message is already locked
BOOL Message::hasLock(VOID)
{
  SBYTE * spool_dir = config.getValue(KEY_SERVER_SPOOL_DIR);
  Config * lock = NULL;
  STRING lockfile;
  BOOL haslock;

  DEBUGGER("Message::hasLock()");
  ASSERT(name.size());
  ASSERT(spool_dir);

  lock = new Config("lock");
  ASSERT(lock);

  lock->initLockKeys();
  lock->clear();
  lock->setDefaults();

  lockfile = string(spool_dir) + name  + FILE_LOCK_EXT;
  haslock = lock->read(lockfile.c_str(), TRUE);

  if (haslock == TRUE)
  {
    // check if logfile should be deleted again / has expired / scheduler kills task
    if (timer.getUnix() > lock->getIntValue(KEY_LOCK_DATE) + MAX_TASK_TIME)
    {
      log.add(1,"error: lock-file for message '%s' has expired, deleted", name.c_str());
      unlock(lockfile.c_str());
      haslock = FALSE;
    }
  }

  delete lock;
  return(haslock);
}

// make a new message
Message * Message::clone(const SBYTE * lname, Address * nrec = NULL)
{
  SBYTE * spool_dir = config.getValue(KEY_SERVER_SPOOL_DIR);
  Address * snd = NULL, * rec = NULL;
  Message * cmsg = NULL;
  STRING nam, src, dst;

  DEBUGGER("Message::clone()");
  ASSERT(spool_dir);
  ASSERT(lname);
  ASSERT(task);
  ASSERT(sender);
  ASSERT(currAdd);

  nam = name + "-" + task->getSPid() + "-" + getNextId();
  src = string(spool_dir) + name + FILE_DATA_EXT;
  dst = string(spool_dir) + nam + FILE_DATA_EXT;

  cmsg = new Message();
  ASSERT(cmsg);

  cmsg->init();
  cmsg->setName(nam);
  cmsg->setSize(size);
  cmsg->lock();
  ASSERT(cmsg->mconf);

  // make hardlink for message-file
  if (link(src.c_str(), dst.c_str()))
  {
    log.add(2,"error: creation of hardlink '%s' -> '%s' failed",
              src.c_str(), dst.c_str());
    cmsg->unlock();
    delete cmsg;
    return(NULL);
  }

  // add recipient (should only be one)
  rec = new Address;
  ASSERT(rec);
  if (nrec)
  {
    rec->setUserName(nrec->getUser());
    rec->setDomainName(nrec->getDomain());
  }
  else
  {
    rec->setUserName(currAdd->getUser());
    rec->setDomainName(currAdd->getDomain());
  }
  cmsg->addRec(rec);

  // add sender
  snd = new Address;
  ASSERT(snd);
  snd->setUserName(sender->getUser());
  snd->setDomainName(sender->getDomain());
  cmsg->setSender(snd);

  // set listname
  if (lname)
    cmsg->mconf->addValue(KEY_MESG_LIST_NAME, lname);

  return(cmsg);
}

// (re)- initializing message, deleting old values
VOID Message::init(VOID)
{
  DEBUGGER("Message::init()");

  size = 0;
  name = "";

  if (mconf)
  {
    delete mconf;
    mconf = NULL;
  }

  mconf = new Config("message");
  ASSERT(mconf);

  mconf->initMsgKeys();
  mconf->clear();
  mconf->setDefaults();

  lines.clear();

  // deleting sender
  if (sender)
  {
    delete sender;
    sender = NULL;
  }

  // deleting all recipients
  recipients.clear();
}

// remove message from spool-area
BOOL Message::moveToDone(SBYTE * done)
{
  STRING src, dst;
  SBYTE * spool_dir = config.getValue(KEY_SERVER_SPOOL_DIR);

  DEBUGGER("Message::moveToDone()");
  ASSERT(done);
  ASSERT(spool_dir);

  // mv dat
  src = string(spool_dir) + name + FILE_DATA_EXT;
  dst = string(done) + name + FILE_DATA_EXT;
  if (rename(src.c_str(), dst.c_str()) != 0)
  {
    log.add(2,"error: unable to rename file '%s' to '%s'", src.c_str(), dst.c_str());
    return(FALSE);
  }

  // mv cfg
  src = string(spool_dir) + name + FILE_CONF_EXT;
  dst = string(done) + name + FILE_CONF_EXT;
  if (rename(src.c_str(), dst.c_str()) != 0)
  {
    log.add(2,"error: unable to rename file '%s' to '%s'", src.c_str(), dst.c_str());
    return(FALSE);
  }

  // mv log
  src = string(spool_dir) + name + FILE_LOG_EXT;
  dst = string(done) + name + FILE_LOG_EXT;
  if (rename(src.c_str(), dst.c_str()) != 0)
  {
    log.add(2,"error: unable to rename file '%s' to '%s'", src.c_str(), dst.c_str());
    return(FALSE);
  }

  return(TRUE);
}

// preparing message for next spool-run (save config)
BOOL Message::queue(SBYTE * stat, QOPTIONS opts)
{
  STRING fname, tmp;
  SBYTE  x[MAX_INT_STR_SIZE+1];
  SBYTE  * t = NULL;
  SBYTE * spool_dir = config.getValue(KEY_SERVER_SPOOL_DIR);
  SBYTE * done_dir = config.getValue(KEY_SERVER_DONE_DIR);

  DEBUGGER("Message::queue()");
  ASSERT(stat);
  ASSERT(mconf);
  ASSERT(spool_dir);
  ASSERT(done_dir);

  // sender and at least one rec must be set
  if ((!sender) || (!recipients.getCount()))
  {
    log.add(3,"error: can't queue message, not all addresses specified");
    return(FALSE);
  }

  // set sender
  tmp = string(sender->getUser()) + "@" + sender->getDomain();
  mconf->addValue(KEY_MESG_FROM, tmp.c_str());

  // writing message logfile
  fname = string(spool_dir) + name + FILE_LOG_EXT;
  mlog.setFile(fname.c_str());
  mlog.setDebug(DEBUG_MAX);

  // set size
  snprintf(x, MAX_INT_STR_SIZE, "%ld", size);
  mconf->addValue(KEY_MESG_SIZE, x);

  // increase tries if send failed again
  if (opts & Q_AGAIN)
    tries ++;

  // set tries
  snprintf(x, MAX_INT_STR_SIZE, "%ld", tries);
  mconf->addValue(KEY_MESG_TRIES, x);

  // set those entries only the first time (initial)
  if (opts & Q_INIT)
  {
    mconf->addValue(KEY_MESG_INIT_DATE, timer.getUnixString());
    mconf->addValue(KEY_MESG_NAME, name.c_str());

    if ((t = mconf->getValue(KEY_MESG_SEND)))
      mlog.add(9,"received from: %s", t);

    mlog.add(9,"mail from: '%s'", tmp.c_str());
    mlog.add(9,"data: '%s' (%ld bytes)", name.c_str(), size);
  }

  mconf->addValue(KEY_MESG_STAT, stat);
  mconf->addValue(KEY_MESG_CHNG_DATE, timer.getUnixString());
  mlog.add(9,"new status: '%s'", stat);

  // delete all old recipients from config (if any)
  mconf->delValue(KEY_MESG_RCPT);

  // add all recipients
  currAdd = recipients.getFirst();
  while(currAdd)
  {
    tmp = string(currAdd->getUser()) + "@" + currAdd->getDomain();

    if (opts & Q_INIT)
      mlog.add(9,"rcpt to: '%s'", tmp.c_str());

    mconf->addValue(KEY_MESG_RCPT, tmp.c_str());
    currAdd = recipients.getNext(currAdd);
  }
  
  // writing config
  tmp = string(spool_dir) + name + FILE_CONF_EXT;
  mconf->write(tmp.c_str());

  if (opts & Q_DONE)
    moveToDone(done_dir);

  return(TRUE);
}

// load message and config
BOOL Message::unqueue(STRING conf)
{
  STRING fname;
  SBYTE * res = NULL, * rec = NULL;
  SBYTE * spool_dir = config.getValue(KEY_SERVER_SPOOL_DIR);
  ULONG num = 0;

  DEBUGGER("Message::unqueue()");
  ASSERT(conf.size());
  ASSERT(mconf);
  ASSERT(spool_dir);

  // setting name (file,lock,conf etc.)
  setName(conf);

  // preparing logfile
  fname = string(spool_dir) + conf + FILE_LOG_EXT;
  mlog.setFile(fname.c_str());
  mlog.setDebug(DEBUG_MAX);

  // reading configuration file
  fname = string(spool_dir) + conf + FILE_CONF_EXT;
  if (mconf->read(fname.c_str()) == FALSE)
    return(FALSE);

  // setting message size/spool tries
  setSize(mconf->getIntValue(KEY_MESG_SIZE));
  tries = mconf->getIntValue(KEY_MESG_TRIES);

  // setting sender
  sender = new Address;
  ASSERT(sender);
  if ((res = sender->setIfValid(mconf->getValue(KEY_MESG_FROM), FALSE)))
  {
    log.add(7,"error: unqueued sender: %s", res);
    return(FALSE);
  }
  log.add(5,"Message: adding sender: %s@%s", sender->getUser(), sender->getDomain());

  // loop for recipients
  while ((rec = mconf->getNext(++num, KEY_MESG_RCPT)))
  {
    currAdd = new Address;
    ASSERT(currAdd);
    if ((res = currAdd->setIfValid(rec)))
    {
      log.add(3,"error: unqueued sender: %s (%s), deleted", rec, res);
      delete currAdd;
    }
    else
    {
      recipients.add(currAdd);
      log.add(7,"Message: adding recipient: %s", rec);
    }
  }

  // set pointer to first recipient
  currAdd = recipients.getFirst();

  ASSERT(sender);
  ASSERT(currAdd);

  // TODO HELP XXX check if datafile exists
  return(TRUE);
}

// save created message to file
BOOL Message::save(VOID)
{
  STRING fname;
  Storage * stor = NULL;
  DataBlock block(TYPE_LINE);
  Line * nextLine = NULL;
  SBYTE * spool_dir = config.getValue(KEY_SERVER_SPOOL_DIR);

  DEBUGGER("Message::save()");
  ASSERT(lines.getCount());
  ASSERT(spool_dir);

  stor = new FileStorage(STORE_DATA, MODE_WRITE);
  ASSERT(stor);

  // opening datafile and copy buffer into file
  fname = string(spool_dir) + getName() + FILE_DATA_EXT;
  if (stor->open(fname.c_str()) != TRUE)
  {
    mlog.add(1,"info: can't open datafile '%s' for writing", fname.c_str());
    return(FALSE);
  }
  else
    mlog.add(4,"info: opened datafile '%s' for writing", fname.c_str());

  // copy all lines to datablock
  currLine = lines.getFirst();
  while(currLine)
  {
    nextLine = lines.getNext(currLine);
    lines.del(currLine);
    block.add(currLine);
    currLine = nextLine;
  }

  // save the whole block
  if (stor->add(&block) == FALSE)
    mlog.add(1,"info: write to datafile '%s' failed, giving up", fname.c_str());

  stor->close();
  delete stor;
  return(TRUE);
}

// adding a command line
VOID Message::addLine(const SBYTE *fmt, ...)
{
  Line * cur = NULL;
  va_list  ap;

  DEBUGGER("Message::addLine()");
  ASSERT(fmt);
  cur = new Line;
  ASSERT(cur);

  va_start(ap, fmt);
  if (vsnprintf((SBYTE *)&cur->str, MAX_MAIL_COMMAND_SIZE, fmt, ap) == -1)
  {
    mlog.add(3,"warning: message line buffer truncated, ignored");
    delete cur;
    return;
  }
  va_end(ap);
  lines.add(cur);
}

// parsing received mail and return generated answer
BOOL Message::generateAnswer(SBYTE * srcname)
{
  BOOL   body = FALSE;
  ULONG  nlines = 0, cmd_errs = 0, cmd_okay = 0;
  SBYTE  * line = NULL, * str = NULL, * usr = NULL, * dom = NULL;
  Line   * tmpline = NULL;
  Storage * stor = NULL;
  SBYTE * spool_dir = config.getValue(KEY_SERVER_SPOOL_DIR);

  STRING fname;
  DataBlock block(TYPE_LINE);

  DEBUGGER("Message::generateAnswer()");
  ASSERT(srcname);
  ASSERT(sender);
  ASSERT(currAdd);
  ASSERT(spool_dir);

  usr = config.getValue(KEY_SERVER_MAIL);
  dom = config.getValue(KEY_SERVER_DOMAIN);
  str = config.getValue(KEY_SERVER_STRING);

  ASSERT(str);
  ASSERT(usr);
  ASSERT(dom);

  stor = new FileStorage(STORE_DATA, MODE_READ);
  ASSERT(stor);

  // opening datafile
  fname = string(spool_dir) + srcname + FILE_DATA_EXT;
  if (stor->open(fname.c_str()) != TRUE)
  {
    log.add(1, "error: can't open spoolfile '%s' for reading", fname.c_str());
    return(FALSE);
  }
  else
    log.add(4,"info: opened spoolfile '%s' for reading", fname.c_str());

  addLine("Sender: %s <%s@%s>\r\n", str, usr, dom);
  addLine("From: %s <%s@%s>\r\n", str, usr, dom);
  addLine("Reply-To: %s <%s@%s>\r\n", str, usr, dom);
  addLine("Message-ID: <%s%ld@%s>\r\n", getName(), task->getSPid(), dom);
  addLine("Date: %s\r\n", timer.getSMTPDate());
  addLine("To: %s@%s\r\n", currAdd->getUser(), currAdd->getDomain());
  addLine("Subject: %s\r\n", config.getValue(KEY_SERVER_SUBJECT));
  addLine("\r\n");

  // reading line into buffer
  while (stor->get(&block))
  {
    if (!block.get(&tmpline))
      break;

    line = tmpline->str;

    // parse command in subject line
    if ((body == FALSE) && !strncasecmp(line, "subject:", 8))
      if (parseCommand(FALSE, TRUE, line+9))
        cmd_okay ++;
      else
        cmd_errs ++;

    // strip mailheader
    if (strlen(line) > 2 && !body)
      continue;

    body = TRUE;

    //log.add(1,"line[%ld] = %s", strlen(line), line);

    // strip empty lines
    if (strlen(line) == 2)
      continue;

    // single space (netscape mailer)
    if ((strlen(line) == 3) && line[0] == ' ')
      continue;

    // end of mail
    if ((strlen(line) == 3) && line[0] == '.')
      break;

    // don't parse signature
    if ((strlen(line) == 4) && line[0] == '-' && line[1] == '-')
      break;
 
    // maximum lines exceeded
    if (nlines > MAX_MAIL_COMMANDS)
    {
      addLine("command limit reached, rest of mail ignored.\r\n");
      addLine("please do not send more than %d commands in a single e-mail.\r\n", MAX_MAIL_COMMANDS);
      break;
    }

    // look for valid commands in line
    if (parseCommand(FALSE, TRUE, line) == FALSE)
    {
      if (cmd_errs++ > MAX_MAIL_ERRORS)
      {
        log.add(2, "warning: error limit reached, rest of mail ignored");
        addLine("error limit reached, rest of mail ignored.\r\n");
        addLine("maybe you did'nt send the message as plain text.\r\n");
        break;
      }
    }
    else
      cmd_okay++;
  }

  if (lines.getCount() < 9)
  {
    addLine("no valid lines found in mail, ignored.\r\n"); 
    log.add(2, "warning: no valid lines found in mail");
  }

  addLine("\r\n");
  addLine("--\r\n");
  addLine("brought to you by %s (%s)\r\n", LIMEZSTRING, LIMEZURL);
  addLine("total commands: %d, succeeded: %d, failed: %d\r\n", cmd_okay + cmd_errs, cmd_okay, cmd_errs);

  stor->close();
  delete stor;

  if (quiet || config.getBoolValue(KEY_SERVER_COMMAND_QUIET))
    return(FALSE);
  else
    return(TRUE);
}

// parsing commands in line
BOOL Message::parseCommand(BOOL approved, BOOL quote, SBYTE * cmd)
{
  INT x, l;

  DEBUGGER("Message::parseCommand()");
  ASSERT(cmd);
  ASSERT(sender);
  ASSERT(currAdd);

  x = strlen(cmd);
  if (x > MAX_MAIL_COMMAND_SIZE)
  {
    log.add(2,"info: command requested by '%s@%s' exceeds max length",
              currAdd->getUser(), currAdd->getDomain());
    addLine("given command line was longer then %d chars, ignored.\r\n", MAX_MAIL_COMMAND_SIZE);
    return(FALSE); 
  }

  // strip leading whitespaces
  while(*cmd == 0x20 || *cmd == 0x09) cmd++;

  // nothing but whitespaces
  if (*cmd == 0x0d || *cmd == 0x0a)
    return(FALSE);

  // tolower conversion
  for (l=0; l<x; l++)
  {
    if ((*(cmd+l) > 0x40) && (*(cmd+l) < 0x5B))
      *(cmd+l) += 0x20;  
  }

  // first line contains command
  if (quote == TRUE)
    addLine("> %s", cmd);

  // HELP
  if (!strncmp(cmd, "help", 4))
  {
    if (x>6 && *(cmd+5) != 0x20);
    else
    {
      log.add(2,"info: 'help' requested by '%s@%s'", currAdd->getUser(), currAdd->getDomain());
      addLine("help page requested.\r\n");
      addLine("\r\n");
      addLine("valid %s mail commands are:\r\n", LIMEZSTRING);
      addLine("\r\n");
      addLine("app <pass> <cmd> <list> [e-mail] ... approve command with password\r\n");
      addLine("add <list> [e-mail] ................ adding [e-mail] to <list>\r\n");
      addLine("del <list> [e-mail] ................ removing [e-mail] from <list>\r\n");
      addLine("help ............................... this helppage\r\n");
      addLine("quiet .............................. reply mail will be suppressed\r\n");
      addLine("\r\n");
      addLine("the following commands always need to be approved by password:\r\n");
      addLine("\r\n");
//    addLine("lists .............................. showing all configured lists\r\n");
      addLine("addsender <list> <e-mail> .......... add sender [e-mail] to <list>\r\n");
      addLine("delsender <list> <e-mail> .......... del sender [e-mail] from <list>\r\n");
      addLine("\r\n");
      return(TRUE);
    }
  }

  // QUIET
  if (!strncmp(cmd, "quiet", 5))
  {
    log.add(2,"info: 'quiet' requested by '%s@%s'", currAdd->getUser(), currAdd->getDomain());
    quiet = TRUE;
    return(TRUE);
  }    

  // LISTS
  if (!strncmp(cmd, "lists", 5))
  {
    log.add(2,"info: 'lists' requested by '%s@%s'", currAdd->getUser(), currAdd->getDomain());

    if (approved == TRUE)
    {
      for (ml = lists.getFirst(); ml; ml = lists.getNext(ml))
      {
        SBYTE * llname = ml->mlconfig.getValue(KEY_LIST_NAME);
        SBYTE * lldesc = ml->mlconfig.getValue(KEY_LIST_DESCR);

        addLine("%s\t%s\r\n", llname, lldesc?lldesc:"");
      }
      addLine("\r\n");
    }
    else
    {
      addLine("command needs to be approved with password, sorry.\r\n");
      addLine("\r\n");
    }
    return(TRUE);
  }    

  // STATS
  if (!strncmp(cmd, "stats", 5))
  {
    log.add(2,"info: 'stats' requested by '%s@%s'", currAdd->getUser(), currAdd->getDomain());
    addLine("command not implemented yet, sorry.\r\n");
    addLine("\r\n");
    return(TRUE);
  }

  // ADDSENDER
  if (!strncmp(cmd, "addsender", 9))
    return(change(approved, FALSE, CMD_ADDSENDER, "addsender", cmd+9));

  // DELSENDER
  if (!strncmp(cmd, "delsender", 9))
    return(change(approved, FALSE, CMD_DELSENDER, "delsender", cmd+9));

  // ADDUSER
  if (!strncmp(cmd, "adduser", 7))
    return(change(approved, FALSE, CMD_ADDUSER, "adduser", cmd+7));

  // ADD
  if (!strncmp(cmd, "add", 3))
    return(change(approved, FALSE, CMD_ADDUSER, "add", cmd+3));

  // INSERT
  if (!strncmp(cmd, "insert", 6))
    return(change(approved, FALSE, CMD_ADDUSER, "insert", cmd+6));

  // SUBSCRIBE
  if (!strncmp(cmd, "subscribe", 9))
    return(change(approved, FALSE, CMD_ADDUSER, "subscribe", cmd+9));

  // SUB
  if (!strncmp(cmd, "sub", 3))
    return(change(approved, FALSE, CMD_ADDUSER, "sub", cmd+3));

  // UNSUBSCRIBE
  if (!strncmp(cmd, "unsubscribe", 11))
    return(change(approved, FALSE, CMD_DELUSER, "unsubscribe", cmd+11));

  // UNSUB
  if (!strncmp(cmd, "unsub", 5))
    return(change(approved, FALSE, CMD_DELUSER, "unsub", cmd+5));

  // DELETE
  if (!strncmp(cmd, "delete", 6))
    return(change(approved, FALSE, CMD_DELUSER, "delete", cmd+6));

  // DELUSER
  if (!strncmp(cmd, "deluser", 7))
    return(change(approved, FALSE, CMD_DELUSER, "deluser", cmd+7));

  // DEL
  if (!strncmp(cmd, "del", 3))
    return(change(approved, FALSE, CMD_DELUSER, "del", cmd+3));

  // REMOVE
  if (!strncmp(cmd, "remove", 6))
    return(change(approved, FALSE, CMD_DELUSER, "remove", cmd+6));

  // REM
  if (!strncmp(cmd, "rem", 3))
    return(change(approved, FALSE, CMD_DELUSER, "rem", cmd+3));

  // APPROVE
  if (!strncmp(cmd, "approve", 7))
    return(approve("approve", cmd+7));

  // APP
  if (!strncmp(cmd, "app", 3))
    return(approve("app", cmd+3));

  // XADD (don't care about welcome/bye-message and do not inform admin)
  if (!strncmp(cmd, "xadd", 4))
    return(change(approved, TRUE, CMD_ADDUSER, "xadd", cmd+4));

  // XDEL (don't care about welcome/bye-message and do not inform admin)
  if (!strncmp(cmd, "xdel", 4))
    return(change(approved, TRUE, CMD_DELUSER, "xdel", cmd+4));

  log.add(2,"info: unknown command sent by '%s@%s': %s",
            currAdd->getUser(), currAdd->getDomain(), cmd);
  addLine("command unrecognized, ignored.\r\n");
  addLine("\r\n");
  return(FALSE);
}

// change user/sender storage
BOOL Message::change(BOOL approved, BOOL option, CMDTYPE type, SBYTE * cname, SBYTE * cmd)
{
  Address * tryrec = NULL;
  SBYTE * result = NULL, listn[MAX_SMTP_USER_SIZE+1], mail[MAX_SMTP_ADDR_SIZE+1];
  INT y = 0;
  BOOL ok = FALSE;

  DEBUGGER("Message::subscribe()");
  ASSERT(cname);
  ASSERT(cmd);
  ASSERT(sender);
  ASSERT(currAdd);

  log.add(6,"info: changing '%s', '%s'", cname, cmd);

  // test command
  if (isalnum(*cmd))
  {
    log.add(2,"info: unknown command sent by '%s@%s': %s",
              currAdd->getUser(), currAdd->getDomain(), cname);
    addLine("given command unrecognized, ignored.\r\n");
    addLine("\r\n");
    return(FALSE);
  }

  // make sure there are more commands to come
  if ((*cmd != 0x20) && (*cmd != 0x09))
  {
    log.add(2,"info: command sent by '%s@%s' failed (missing arguments): %s",
              currAdd->getUser(), currAdd->getDomain(), cname);
    addLine("%s requires arguments.\r\n", cname);
    addLine("\r\n");
    return(FALSE);
  }

  // strip leading whitespaces
  while(*cmd == 0x20 || *cmd == 0x09) cmd++;

  // copying listname
  while(*cmd != 0x20 && *cmd != 0x09 &&
        *cmd != 0x0d && *cmd != 0x0a &&
        y < MAX_SMTP_USER_SIZE)
    listn[y++]= *(cmd++);
    
  listn[y] = '\0';

  // get list
  if (!(ml = getList(listn)))
  {
    log.add(2,"info: '%s@%s' tried to access (add/del) unknown list '%s'",
              currAdd->getUser(), currAdd->getDomain(), listn);
    addLine("unknown listname, ignored.\r\n");
    addLine("\r\n");
    return(FALSE);
  }

  // strip leading whitespaces
  while(*cmd == 0x20 || *cmd == 0x09) cmd++;

  // copying mailaddress
  y = 0;
  while(*cmd != 0x20 && *cmd != 0x09 &&
        *cmd != 0x0d && *cmd != 0x0a &&
        *cmd != 0x00 && y < MAX_SMTP_ADDR_SIZE)
    mail[y++]= *(cmd++);

  mail[y] = '\0';

  // no e-mail given ... trying sender
  if (!strlen(mail))
    snprintf(mail, MAX_SMTP_ADDR_SIZE, "%s@%s", currAdd->getUser(), currAdd->getDomain());

  // check & set e-mail
  tryrec = new Address;
  ASSERT(tryrec);

  result = tryrec->setIfValid(mail);
  if (result != NULL)
  {
    ml->mllog.add(2,"info: user '%s': check failed '%s' (%s)", mail, listn, result);
    addLine("%s, ignored.\r\n", result);
    addLine("\r\n");
    delete tryrec;
    return(TRUE);
  }

  // test if recipient is one of our lists (mailserver)
  if (tryrec->isValidRec() != REC_NONE)
  {
    ml->mllog.add(2,"info: user '%s@%s' not added to/removed from list '%s' (is one of our lists/aliases)",
                    tryrec->getUser(), tryrec->getDomain(), listn);
    addLine("user is one of our lists/aliases, ignored.\r\n");
    addLine("\r\n");
    delete tryrec;
    return(TRUE);
  }

  // invoke desired command
  if (type == CMD_ADDUSER || type == CMD_DELUSER)
    ok = changeuser(approved, option, type, tryrec);
  else
    ok = changesender(approved, option, type, tryrec);

  delete tryrec;
  return(ok);
}

// un/subscribing user
BOOL Message::changeuser(BOOL approved, BOOL option, CMDTYPE type, Address * rec)
{
  SBYTE * sendType = NULL, * msgfile = NULL;
  SBYTE * listn = NULL, * admin = NULL;
  BOOL inform;

  DEBUGGER("Message::changeuser()");
  ASSERT(sender);
  ASSERT(rec);
  ASSERT(ml);
  ASSERT(task);
  ASSERT(currAdd);

  inform = ml->mlconfig.getBoolValue(KEY_LIST_INFORM_ADMIN);
  admin = ml->mlconfig.getValue(KEY_LIST_ADMIN);
  listn = ml->mlconfig.getValue(KEY_LIST_NAME);
  ASSERT(listn);

  if (option == FALSE)
  {
    // check if sender is authorized
    if (type == CMD_ADDUSER)
      sendType = ml->mlconfig.getValue(KEY_LIST_SUBSCRIBE);
    else
      sendType = ml->mlconfig.getValue(KEY_LIST_UNSUBSCRIBE);

    if (!sendType)
    {
      ml->mllog.add(3,"warning: 'list_%s' not defined, assuming 'closed'",
                      (type == CMD_ADDUSER)?"subscribe":"unsubscribe");
      return(FALSE);
    }

    // closed - never add/del
    if (!strcasecmp(sendType, "closed"))
    {
      ml->mllog.add(2,"info: user '%s@%s' not added to list '%s' ('closed')",
                      rec->getUser(), rec->getDomain(), listn);
      addLine("not authorized, please contact the list administrator (mailto:%s).\r\n",
              ml->mlconfig.getValue(KEY_LIST_ADMIN));
      addLine("\r\n");
      return(FALSE);
    }

    // check if action needs to be approved
    if (approved == FALSE && !strcasecmp(sendType, "approve"))
    {
      ml->mllog.add(2,"info: user '%s@%s' not added to list '%s' (not approved)",
                      rec->getUser(), rec->getDomain(), listn);
      addLine("must be approved, please contact the list administrator (mailto:%s).\r\n",
              ml->mlconfig.getValue(KEY_LIST_ADMIN));
      addLine("\r\n");
      return(FALSE);
    }

    // check if someone tried to add/del someone else
    if (approved == FALSE && !strcasecmp(sendType, "open") && rec->compare(currAdd) == FALSE)
    {
      ml->mllog.add(2,"info: user '%s@%s' not added to list '%s' (addresses differ)",
                      rec->getUser(), rec->getDomain(), listn);
      addLine("e-mail addresses differ, please contact the administrator (mailto:%s).\r\n",
              ml->mlconfig.getValue(KEY_LIST_ADMIN));
      addLine("\r\n");
      return(FALSE);
    }
  }
  // is user already/not in list ?
  if (ml->isUserMember(rec) == TRUE)
  {
    if (type == CMD_ADDUSER)
    {
      ml->mllog.add(2,"info: user '%s@%s' is already subscribed to list '%s'",
                      rec->getUser(), rec->getDomain(), listn);

      addLine("user already subscribed, ignored.\r\n");
      addLine("\r\n");
      return(FALSE);
    }
  }
  else
  {
    if (type == CMD_DELUSER)
    {
      ml->mllog.add(2,"info: remove from '%s@%s' failed, not user of '%s'",
                      rec->getUser(), rec->getDomain(), listn);

      addLine("user not subscribed, ignored.\r\n");
      addLine("\r\n");
      return(FALSE);
    }
  }

  // un/subscribing user
  if (type == CMD_ADDUSER)
  {
    if (ml->addUser(rec) == TRUE)
    {
      ml->mllog.add(2,"info: user '%s@%s' has been subscribed to '%s'",
                      rec->getUser(), rec->getDomain(), listn);
    }
    else
    {
      ml->mllog.add(2,"error: can't store '%s@%s' in '%s'",
                      rec->getUser(), rec->getDomain(), listn);

      addLine("internal error, not succeeded.\r\n");
      addLine("\r\n");
      return(FALSE);
    }
  }
  else
  {
    if (ml->delUser(rec) == TRUE)
    {
      ml->mllog.add(2,"info: user '%s@%s' has been unsubscribed from '%s'",
                      rec->getUser(), rec->getDomain(), listn);
    }
    else
    {
      ml->mllog.add(2,"error: can't remove '%s@%s' from '%s'",
                      rec->getUser(), rec->getDomain(), listn);

      addLine("internal error, not succeeded.\r\n");
      addLine("\r\n");
      return(FALSE);
    }
  }

  // do not check for welcome/bye - message and do not send message to admin
  if (option == TRUE)
  {
    if (type == CMD_ADDUSER)
      addLine("succeeded, user added to list\r\n");
    else
      addLine("succeeded, user removed from list\r\n");
  }
  else
  {
    // sending welcome/goodbye - message if configured
    if (type == CMD_ADDUSER)
      msgfile = ml->mlconfig.getValue(KEY_LIST_WELCOME);
    else
      msgfile = ml->mlconfig.getValue(KEY_LIST_GOODBYE);

    // generate Welcome/bye - message
    if (msgfile)
    {
      creator.welcomeBye(msgfile, rec);

      if (type == CMD_ADDUSER)
        addLine("succeeded, user added to list, sending welcome message.\r\n");
      else
        addLine("succeeded, user removed from list, sending goodbye message.\r\n");
    }
    else
    {
      if (type == CMD_ADDUSER)
        addLine("succeeded, user added to list without welcome message.\r\n");
      else
        addLine("succeeded, user removed from list without welcome message.\r\n");
    }

    // send mail to listadmin if requested
    if (inform && admin)
      creator.informAdmin((type == CMD_ADDUSER)?TRUE:FALSE, listn, admin, rec);
  }
  addLine("\r\n");
  return(TRUE);
}

// add / delete sender
BOOL Message::changesender(BOOL approved, BOOL option, CMDTYPE type, Address * rec)
{
  SBYTE * listn = NULL;

  DEBUGGER("Message::changesender()");

  ASSERT(sender);
  ASSERT(rec);
  ASSERT(ml);

  listn = ml->mlconfig.getValue(KEY_LIST_NAME);
  ASSERT(listn);

  if (approved == FALSE)
  {
    addLine("command needs to be approved with password, ignored.\r\n");
    addLine("\r\n");
    return(FALSE);
  }

  // check if user is already sender
  if (ml->isUserSender(rec) == TRUE)
  {
    if (type == CMD_ADDSENDER)
    {
      ml->mllog.add(2,"info: user '%s@%s' is already sender of list '%s'",
                      rec->getUser(), rec->getDomain(), listn);

      addLine("user is already sender, ignored.\r\n");
      addLine("\r\n");
      return(FALSE);
    }
  }
  else
  {
    if (type == CMD_DELSENDER)
    {
      ml->mllog.add(2,"info: remove from '%s@%s' failed, not sender of '%s'",
                      rec->getUser(), rec->getDomain(), listn);

      addLine("user is not sender, ignored.\r\n");
      addLine("\r\n");
      return(FALSE);
    }
  }

  // un/subscribing sender
  if (type == CMD_ADDSENDER)
  {
    if (ml->addSender(rec) == TRUE)
    {
      ml->mllog.add(2,"info: user '%s@%s' has been added as sender of '%s'",
                      rec->getUser(), rec->getDomain(), listn);
      addLine("succeeded, user has been added as sender.\r\n");
      addLine("\r\n");
    }
    else
    {
      ml->mllog.add(1,"error: can't store '%s@%s' as sender in '%s'",
                      rec->getUser(), rec->getDomain(), listn);
      addLine("internal error, not succeeded.\r\n");
      addLine("\r\n");
      return(FALSE);
    }
  }
  else
  {
    if (ml->delSender(rec) == TRUE)
    {
      ml->mllog.add(2,"info: user '%s@%s' has been removed as sender from '%s'",
                      rec->getUser(), rec->getDomain(), listn);
      addLine("succeeded, user has been removed from senders.\r\n");
      addLine("\r\n");
    }
    else
    {
      ml->mllog.add(1,"error: can't remove '%s@%s' as sender from '%s'",
                      rec->getUser(), rec->getDomain(), listn);
      addLine("internal error, not succeeded.\r\n");
      addLine("\r\n");
      return(FALSE);
    }
  }

  // HELP TODO XXX send mail to listadmin if requested
  return(TRUE);
}

// approve command, requires password
BOOL Message::approve(SBYTE * cname, SBYTE * pass)
{
  UWORD y = 0;
  SBYTE pwd[MAX_PASS_SIZE+1];
  SBYTE * lname = NULL, * end = NULL;
  MailingList * ml = NULL;

  DEBUGGER("Message::approve()");

  ASSERT(cname);
  ASSERT(pass);
  ASSERT(sender);
  ASSERT(currAdd);

  if (isalnum(*pass))
  {
    log.add(2,"info: unknown command from '%s@%s', approve failed",
              currAdd->getUser(), currAdd->getDomain());
    addLine("command unrecognized, ignored.\r\n");
    addLine("\r\n");
    return(FALSE);
  }

  // make sure there are more commands to come
  if ((*pass != 0x20) && (*pass != 0x09))
  {
    log.add(2,"info: approve given without arguments '%s@%s'",
              currAdd->getUser(), currAdd->getDomain());
    addLine("%s requires password and arguments.\r\n", cname);
    addLine("\r\n");
    return(FALSE);
  }

  // strip leading whitespaces
  while(*pass == 0x20 || *pass == 0x09) pass++;

  // copying password
  while(*pass != 0x20 && *pass != 0x09 &&
        *pass != 0x0d && *pass != 0x0a &&
        y < MAX_PASS_SIZE)
    pwd[y++]= *(pass++);

  pwd[y] = '\0';

  lname = pass;

  // strip leading whitespaces
  while(*lname == 0x20 || *lname == 0x09) lname++;

  // ignore command, the list is what we're looking for
  while(*lname != 0x20 && *lname != 0x09 &&
        *lname != 0x0d && *lname != 0x0a)
    lname++;

  // strip leading whitespaces
  while(*lname == 0x20 || *lname == 0x09) lname++;

  end = lname;

  // get end of listname
  while(*end != 0x20 && *end != 0x09 &&
        *end != 0x0d && *end != 0x0a)
    end++;

  *end = '\0';

  // get listname
  if (!(ml = getList(lname)))
  {
    log.add(2,"info: approve from '%s@%s' failed, unknown list '%s'",
              currAdd->getUser(), currAdd->getDomain(), lname);
    addLine("unknown listname '%s', ignored.\r\n", lname);
    addLine("\r\n");
    return(FALSE);
  }

  *end = ' ';

  // comparing list password or try to parse commands
  if (!strcmp(pwd, ml->mlconfig.getValue(KEY_LIST_PASS)))
    return(parseCommand(TRUE, FALSE, pass));

  ml->mllog.add(2,"info: approve from '%s@%s' failed, wrong passord",
                  currAdd->getUser(), currAdd->getDomain());
  addLine("password mismatch, ignored.\r\n");
  addLine("\r\n");
  return(FALSE);
}
