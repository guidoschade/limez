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

#include "generator_def.h"
#include "task_def.h"
#include "datablock_inl.h"
#include "message_inl.h"

extern Task * task;
extern Log log;
extern Config config;
extern Timer timer;

Generator::Generator()
{
}

Generator::~Generator()
{
}

// create new message and prepare header
inline Message * Generator::prepare(Address * rec, BOOL fullheader = TRUE)
{
  SBYTE * usr = NULL, * dom = NULL, * str = NULL;
  Message * msg = new Message();
  STRING newnam;

  DEBUGGER("Generator::prepare()");
  ASSERT(msg);
  ASSERT(rec);

  usr = config.getValue(KEY_SERVER_MAIL);
  dom = config.getValue(KEY_SERVER_DOMAIN);
  str = config.getValue(KEY_SERVER_STRING);

  ASSERT(usr);
  ASSERT(dom);
  ASSERT(str);

  newnam = string(timer.getRDate()) + "-" + task->getSPid() + "-" + getNextId();
  msg->init();
  msg->setName(newnam);

  if (fullheader)
  {
    msg->addLine("Sender: %s <%s@%s>\r\n", str, usr, dom);
    msg->addLine("From: %s <%s@%s>\r\n", str, usr, dom);
    msg->addLine("Reply-To: %s <%s@%s>\r\n", str, usr, dom);
  }

  msg->addLine("Message-ID: <%s@%s>\r\n", newnam.c_str(), dom);
  msg->addLine("Date: %s\r\n", timer.getSMTPDate());
  msg->addLine("To: %s@%s\r\n", rec->getUser(), rec->getDomain());
  return(msg);
}

// save and queue message
inline VOID Generator::finish(Message * msg, Address * rec)
{
  Address * snd = NULL, * adr = NULL;

  DEBUGGER("Generator::finish()");
  ASSERT(rec);
  ASSERT(msg);

  // sender
  snd = new Address;
  ASSERT(snd);
  snd->setUserName(config.getValue(KEY_SERVER_MAIL));
  snd->setDomainName(config.getValue(KEY_SERVER_DOMAIN));

  // recipient
  adr = new Address;
  ASSERT(adr);
  adr->setUserName(rec->getUser());
  adr->setDomainName(rec->getDomain());

  msg->addRec(adr);
  msg->setSender(snd);

  msg->lock();
  msg->save();
  msg->queue("created", Q_INIT);
  msg->unlock();
  delete snd;
}

// generate welcome/bye message
VOID Generator::welcomeBye(SBYTE * fname, Address * rec)
{
  Message * msg = NULL;
  Storage * stor = NULL;
  DataBlock block(TYPE_LINE);
  Line * tmp = NULL;
  SBYTE * c = NULL;
  STRING newnam;

  DEBUGGER("Generator::welcomeBye()");

  ASSERT(fname);
  ASSERT(task);
  ASSERT(rec);

  msg = prepare(rec, FALSE);
  ASSERT(msg);

  stor = new FileStorage(STORE_DATA, MODE_READ);
  ASSERT(stor);

  if (stor->open(fname) == FALSE)
  {
    log.add(2,"error: failed to read messagefile '%s'", fname);
    delete stor;
    return;
  }
  log.add(4,"info: opened messagefile '%s' for reading", fname);

  while (stor->get(&block) == TRUE)
    while (block.get(&tmp) == TRUE)
    {
      // cut return/linefeed
      c = tmp->str;
      while(*c != 0x0d && *c != 0x0a && *c !='\0')
        c++;
      *c = '\0';
      strcat(tmp->str, "\r\n");
      msg->addLine(tmp->str);
    }

  stor->close();
  delete stor;

  finish(msg, rec);
  delete msg;
}

// inform administrator of list about subscribes/unsubs
VOID Generator::informAdmin(BOOL add, SBYTE * list, SBYTE * admin, Address * adr)
{
  Message * msg = NULL;
  Address * rec = NULL;

  DEBUGGER("Generator::informAdmin()");

  ASSERT(task);
  ASSERT(adr);
  ASSERT(list);
  ASSERT(admin);

  // recipient
  rec = new Address;
  ASSERT(rec);
  if (rec->setIfValid(admin))
  {
    delete rec;
    log.add(2,"error: admin '%s' is not valid, cannot send mail");
    return;
  }

  msg = prepare(rec);
  ASSERT(msg);

  msg->addLine("Subject: %s %s@%s\r\n", (add == TRUE)?"add":"del", adr->getUser(), adr->getDomain());
  msg->addLine("\r\n");
  msg->addLine("user '%s@%s' has been %s list '%s'\r\n", adr->getUser(), adr->getDomain(),
               (add == TRUE)?"added to":"removed from", list);
  msg->addLine("no action is required on your part.\r\n");

  finish(msg, rec);
  delete rec;
  delete msg;
}

// first message before broadcast to sender
VOID Generator::preBroadcast(SBYTE * list, ULONG users, Address * rec)
{
  Message * msg = NULL;

  DEBUGGER("Generator::preBroadcast()");
  ASSERT(list);
  ASSERT(rec);

  msg = prepare(rec);
  ASSERT(msg);

  msg->addLine("Subject: %s\r\n", config.getValue(KEY_SERVER_SUBJECT));
  msg->addLine("\r\n");
  msg->addLine("Your broadcast message to list '%s' has been accepted, the e-mail\r\n", list);
  msg->addLine("will be sent to %ld subscribed users. You will receive another message\r\n", users);
  msg->addLine("after broadcast containing delivery statistics.\r\n");
  msg->addLine("\r\n");
  msg->addLine("\r\n");
  msg->addLine("--\r\n");
  msg->addLine("brought to you by %s (%s)\r\n", LIMEZSTRING, LIMEZURL);

  finish(msg, rec);
  delete msg;
}

// non member broadcast
VOID Generator::nonMemberBroadcast(SBYTE * list, SBYTE * policy, SBYTE * admin, Address * rec)
{
  Message * msg = NULL;
  Address * adm = NULL;

  DEBUGGER("Generator::nonMemberBroadcast()");
  ASSERT(list);
  ASSERT(policy);
  ASSERT(admin);
  ASSERT(rec);

  msg = prepare(rec);
  ASSERT(msg);

  msg->addLine("Subject: Non Member Broadcast\r\n");
  msg->addLine("\r\n");
  msg->addLine("You are not allowed to broadcast to the mailinglist '%s' (policy '%s').\r\n", list, policy);
  msg->addLine("Please contact the list administrator (mailto:%s), if you're not\r\n", admin);
  msg->addLine("comfortable with this mail.\r\n");
  msg->addLine("\r\n");
  msg->addLine("The administrator of this mailinglist has been notified.\r\n");
  msg->addLine("\r\n");
  msg->addLine("--\r\n");
  msg->addLine("brought to you by %s (%s)\r\n", LIMEZSTRING, LIMEZURL);

  finish(msg, rec);
  delete msg;

  // recipient
  adm = new Address;
  ASSERT(adm);
  if (adm->setIfValid(admin))
  {
    delete adm;
    log.add(2,"error: admin '%s' is not valid, cannot send mail");
    return;
  }

  msg = prepare(adm);
  ASSERT(msg);

  msg->addLine("Subject: Non Member Broadcast\r\n");
  msg->addLine("\r\n");
  msg->addLine("someone ('%s@%s') tried to broadcast to list '%s' (policy '%s').\r\n",
               rec->getUser(), rec->getDomain(), list, policy);
  msg->addLine("it's up to you to change this issue.\r\n");
  msg->addLine("--\r\n");
  msg->addLine("brought to you by %s (%s)\r\n", LIMEZSTRING, LIMEZURL);

  finish(msg, adm);
  delete adm;
  delete msg;
}

// last message after broadcast to sender
VOID Generator::pastBroadcast(SBYTE * list, ULONG users, ULONG sent, ULONG spooled, ULONG unsubs, ULONG secs, ULONG bytes, Address * rec)
{
  Message * msg = NULL;

  DEBUGGER("Generator::pastBroadcast()");
  ASSERT(list);
  ASSERT(rec);

  msg = prepare(rec);
  ASSERT(msg);

  msg->addLine("Subject: %s\r\n", config.getValue(KEY_SERVER_SUBJECT));
  msg->addLine("\r\n");
  msg->addLine("Your broadcast message has been delivered.\r\n");
  msg->addLine("\r\n");
  msg->addLine("Statistics:\r\n");
  msg->addLine("\r\n");
  msg->addLine("total number of users    : %ld\r\n", users);
  msg->addLine("successfully sent        : %ld\r\n", sent);
  //msg->addLine("deferred sent (spooled)  : %ld\r\n", spooled);
  //msg->addLine("unsubscribed (automated) : %ld\r\n", unsubs);
  msg->addLine("\r\n");
  msg->addLine("total send time          : %ld minutes\r\n", (ULONG)((double) secs/60));
  msg->addLine("total bytes sent         : %ld\r\n", bytes * sent);
  msg->addLine("\r\n");
  msg->addLine("Deferred mails will be tried to deliver again during the next hours.\r\n");
  msg->addLine("\r\n");
  msg->addLine("--\r\n");
  msg->addLine("brought to you by %s (%s)\r\n", LIMEZSTRING, LIMEZURL);

  finish(msg, rec);
  delete msg;
}

// maximum size exceeded
VOID Generator::maxSize(SBYTE * list, Address * rec, ULONG size, SBYTE * admin)
{
  Message * msg = NULL;

  DEBUGGER("Generator::pastBroadcast()");
  ASSERT(list);
  ASSERT(rec);

  msg = prepare(rec);
  ASSERT(msg);

  msg->addLine("Subject: Error: mailsize exceeded\r\n");
  msg->addLine("\r\n");
  msg->addLine("Your e-mail exceeds the maximum size (%ld bytes) of mailinglist '%s'.\r\n", size, list);
  msg->addLine("Please contact the list administrator (mailto:%s), if you're not\r\n", admin);
  msg->addLine("comfortable with this mail.\r\n");
  msg->addLine("\r\n");
  msg->addLine("The administrator of this mailinglist has been notified.\r\n");
  msg->addLine("\r\n");
  msg->addLine("--\r\n");
  msg->addLine("brought to you by %s (%s)\r\n", LIMEZSTRING, LIMEZURL);

  finish(msg, rec);
  delete msg;
}

// sending error-report to admin
VOID Generator::sendError(Address * rec, SBYTE * admin)
{
  Message * msg = NULL;
  Address * adm = NULL;

  DEBUGGER("Generator::sendError()");
  ASSERT(rec);
  ASSERT(admin);

  // recipient
  adm = new Address;
  ASSERT(adm);
  if (adm->setIfValid(admin))
  {
    delete adm;
    log.add(2,"error: admin '%s' is not valid, cannot send mail");
    return;
  }

  msg = prepare(adm);
  ASSERT(msg);

  msg->addLine("Subject: Undelivered Mail\r\n");
  msg->addLine("\r\n");
  msg->addLine("The following recipient could not be reached:\r\n");
  msg->addLine("\r\n");
  msg->addLine("%s@%s\r\n", rec->getUser(), rec->getDomain());
  msg->addLine("\r\n");
  msg->addLine("The spoolfile will be deleted.\r\n");

  finish(msg, adm);
  delete adm;
  delete msg;
}
