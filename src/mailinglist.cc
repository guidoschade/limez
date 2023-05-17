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

#include "mailinglist_def.h"
#include "address_inl.h"
#include "datablock_inl.h"
#include "ldapstorage_def.h"
#include "log_inl.h"
#include "task_def.h"

extern Log log;
extern Config config;
extern Task * task;

MailingList::MailingList()
:mlconfig("mailinglist")
{
}

MailingList::~MailingList()
{
}

// delete current mailinglist
BOOL MailingList::remove()
{
  SBYTE * group = mlconfig.getValue(KEY_LIST_LDAP_SENDER_GROUP);
  SBYTE * ssrc = mlconfig.getValue(KEY_LIST_SENDER_SOURCE);
  SBYTE * lnam = mlconfig.getValue(KEY_LIST_NAME);
  SBYTE * lcfg = mlconfig.getValue(KEY_LIST_CONFIG);
  Storage * stor = NULL;

  MLDEBUGGER("MailingList::remove()");
  ASSERT(lnam);
  ASSERT(lcfg);

  // delete config
  mlconfig.remove(lcfg);

  // delete sender
  if (ssrc)
  {
    // define storage type
    if (!strcasecmp(ssrc, USE_LDAP_STORAGE))
    {
      stor = new LDAPStorage(STORE_SENDER, MODE_DELETE);
      ssrc = group?group:lnam;
    }
    else
    {
      if (!strcasecmp(ssrc, USE_DB_STORAGE))
      {
        stor = new DBStorage(STORE_SENDER, MODE_DELETE);
        ssrc = lnam;
      }
      else
        stor = new FileStorage(STORE_SENDER, MODE_DELETE);
    }
    ASSERT(stor);

    if (stor->open(ssrc) == TRUE)
    {
      stor->destroy(ssrc);
      stor->close();
    }
    else
      mllog.add(4,"warning: cannot open source '%s', deletion failed", ssrc);

    delete stor;
  }
  else
    mllog.add(4,"warning: no sendersource for '%s' specified, ignored", ssrc);

  return(TRUE);
}

// initialize configuration & log
BOOL MailingList::initConfig(SBYTE * clist, BOOL db = FALSE)
{
  SBYTE * lnam = NULL;

  DEBUGGER("MailingList::initConfig()");
  ASSERT(clist);

  mlconfig.clear();
  mlconfig.initListKeys();
  mlconfig.setDefaults();

  // if we are using database-config the value is the name
  if (db == TRUE)
  {
    log.add(8,"info: getting mailinglist-config from database");
    mlconfig.setName(clist);
    mlconfig.addValue(KEY_LIST_NAME, clist, CONF_PROT);

    if (mlconfig.read(USE_DB_STORAGE)== FALSE)
    {
      log.add(2,"info: empty listconfig detected, has been filled with default values");
      mlconfig.write(USE_DB_STORAGE);
    }
  }
  else
  {
    log.add(8,"info: getting mailinglist-config from file '%s'", clist);
    mlconfig.read(clist);
    mlconfig.addValue(KEY_LIST_CONFIG, clist);

    if ((lnam = mlconfig.getValue(KEY_LIST_NAME)))
      mlconfig.setName(lnam);
  }

  if (config.getBoolValue(KEY_DEBUG_TEST_ONLY) == TRUE)
    mlconfig.dump();

  if (mlconfig.check() == FALSE)
  {
    log.add(1,"error: mailinglist configuration ('%s') check failed, exiting.", clist);
    exit(1);
  }

  // setting logfile & debug
  mllog.setFile(mlconfig.getValue(KEY_LOGFILE));
  mllog.setDebug(mlconfig.getIntValue(KEY_DEBUG_LEVEL));

  return(TRUE);
}

// reading all sender from source
BOOL MailingList::getAllSender(DataBlock * b)
{
  SBYTE * group = mlconfig.getValue(KEY_LIST_LDAP_SENDER_GROUP);
  SBYTE * src = mlconfig.getValue(KEY_LIST_SENDER_SOURCE);
  SBYTE * lnam = mlconfig.getValue(KEY_LIST_NAME);

  Address * adr = NULL;
  Storage * stor = NULL;
  DataBlock block(TYPE_ADDRESS);

  MLDEBUGGER("MailingList::readSender()");
  ASSERT(lnam);
  ASSERT(b);
  ASSERT(b->getType() == TYPE_ADDRESS);

  if (!src)
  {
    mllog.add(4,"warning: no senderfile for '%s' specified, ignored", lnam);
    return(FALSE);
  }
  
  // define storage type
  if (!strcasecmp(src, USE_LDAP_STORAGE))
  {
    stor = new LDAPStorage(STORE_SENDER, MODE_READ);
    src = group?group:lnam;
  }
  else
  {
    if (!strcasecmp(src, USE_DB_STORAGE))
    {
      stor = new DBStorage(STORE_SENDER, MODE_READ);
      src = lnam;
    }
    else
      stor = new FileStorage(STORE_SENDER, MODE_READ);
  }
  ASSERT(stor);
  if (stor->open(src) == TRUE)
  {
    if (stor->get(&block) == TRUE)
    {
      while (block.get(&adr) == TRUE)
      {
        mllog.add(3,"info: adding sender '%s@%s' to list '%s'", adr->getUser(), adr->getDomain(), lnam);
        b->add(adr);
      }
    }
    else
      mllog.add(5,"warning: can't read sender-storage '%s' for list '%s' (empty ?)", src, lnam);

    stor->close();
    delete stor;
  }
  else
  {
    mllog.add(2,"error: can't read sender-storage '%s' for list '%s'", src, lnam);
    return(FALSE);
  }
  return(TRUE);  
}

// reading all users from source
BOOL MailingList::getAllUsers(DataBlock * b, ULONG num, ULONG count = MAX_USERS_ONCE, BOOL temp = FALSE)
{
  const SBYTE * src = mlconfig.getValue(KEY_LIST_USER_SOURCE);
  SBYTE * table = mlconfig.getValue(KEY_LIST_USERTABLE_NAME);
  SBYTE * group = mlconfig.getValue(KEY_LIST_LDAP_USER_GROUP);
  SBYTE * lnam = mlconfig.getValue(KEY_LIST_NAME);
  STRING  tmpnam;

  ULONG mystart = ((num-1) * count) +1;
  ULONG myend = num * count;

  Address * adr = NULL;
  Storage * stor = NULL;
  DataBlock block(TYPE_ADDRESS);

  MLDEBUGGER("MailingList::getAllUsers()");
  ASSERT(lnam);
  ASSERT(count > 0);
  ASSERT(b);
  ASSERT(b->getType() == TYPE_ADDRESS);

  mllog.add(4,"info: trying to get users for list '%s' from %ld to %ld", lnam, mystart, myend);

  if (!src)
  {
    mllog.add(4,"warning: no usersource for '%s' specified, ignored", src);
    return(FALSE);
  }

  // define storage type
  if (!strcasecmp(src, USE_LDAP_STORAGE))
  {
    temp = FALSE;
    stor = new LDAPStorage(STORE_USER, MODE_READ);
    src = group?group:lnam;
  }
  else
  {
    if (!strcasecmp(src, USE_DB_STORAGE))
    {
      stor = new DBStorage(STORE_USER, MODE_READ);
      src = table?table:lnam;
    }
    else
      stor = new FileStorage(STORE_USER, MODE_READ);
  }
  ASSERT(stor);

  // create temporary user file/table entry for broadcast
  if (temp == TRUE)
  {
    tmpnam = string(src) + "_tmp_" + task->getSPid();
    if (num == 1)
    {
      // open original storage
      if (stor->open(src) == FALSE)
      {
        mllog.add(2,"error: failed to open user storage '%s'", src);
        delete stor;
        return(FALSE);
      }

      // clone storage
      mllog.add(5,"info: creating temporary user storage '%s'", tmpnam.c_str());
      if (stor->clone(tmpnam.c_str()) == FALSE)
      {
        mllog.add(3,"error: failed to create temporary user storage '%s'", tmpnam.c_str());
        stor->close();
        delete stor;
        return(FALSE);
      }
      stor->close();
    }
    src = tmpnam.c_str();
  }

  // open storage (either original or just copy)
  if (stor->open(src) == TRUE)
  {
    if (stor->get(&block, (num-1)*count + 1, count) == TRUE)
    {
      while (block.get(&adr) == TRUE)
      {
        mllog.add(7,"info: got user '%s@%s' from '%s'",
                    adr->getUser(), adr->getDomain(), lnam);
        b->add(adr);
      }
    }
    else
    {
      mllog.add(2,"info: storage source '%s' returned empty set", src);

      // delete tmp-storage again
      if (temp == TRUE)
      {
        mllog.add(5,"info: destroying temporary user storage");
        stor->destroy((SBYTE *) src);
      }

      stor->close();
      delete stor;
      return(FALSE);
    }

    stor->close();
    delete stor;
    return(TRUE);
  }
  else
  {
    mllog.add(2,"error: failed to open user storage '%s'", src);
    delete stor;
    return(FALSE);
  }
}

// look if user is subscribed to mailinglist
BOOL MailingList::isUserMember(Address * rec)
{
  SBYTE * group = mlconfig.getValue(KEY_LIST_LDAP_USER_GROUP);
  SBYTE * table = mlconfig.getValue(KEY_LIST_USERTABLE_NAME);
  SBYTE * src = mlconfig.getValue(KEY_LIST_USER_SOURCE);
  SBYTE * lnam = mlconfig.getValue(KEY_LIST_NAME);

  Address * adr = NULL;
  Storage * stor = NULL;
  DataBlock block(TYPE_ADDRESS);
  BOOL  found = FALSE;

  MLDEBUGGER("MailingList::isUserMember()");
  ASSERT(src);
  ASSERT(rec);
  ASSERT(lnam);

  // define storage type
  if (!strcasecmp(src, USE_LDAP_STORAGE))
  {
    stor = new LDAPStorage(STORE_USER, MODE_READ);
    src = group?group:lnam;
  }
  else
  {
    if (!strcasecmp(src, USE_DB_STORAGE))
    {
      stor = new DBStorage(STORE_USER, MODE_READ);
      src = table?table:lnam;
    }
    else
      stor = new FileStorage(STORE_USER, MODE_READ);
  }
  ASSERT(stor);

  if (stor->open(src) == TRUE)
  {
    adr = new Address;
    ASSERT(adr);
    adr->setUserName(rec->getUser());
    adr->setDomainName(rec->getDomain());

    block.add(adr);
    found = stor->find(&block);
    delete adr;
    stor->close();
  }
  else
    mllog.add(1,"error: reading user storage '%s'", src);

  mllog.add(4,"info: user '%s@%s' %s member of list '%s'",
              rec->getUser(), rec->getDomain(), found == TRUE?"is":"is NOT", lnam);
  delete stor;
  return(found);
}

// look if user is sender of mailinglist
BOOL MailingList::isUserSender(Address * rec)
{
  SBYTE * group = mlconfig.getValue(KEY_LIST_LDAP_SENDER_GROUP);
  SBYTE * src = mlconfig.getValue(KEY_LIST_SENDER_SOURCE);
  SBYTE * lnam = mlconfig.getValue(KEY_LIST_NAME);
  Address * adr = NULL;
  Storage * stor = NULL;
  DataBlock block(TYPE_ADDRESS);
  BOOL  found = FALSE;

  MLDEBUGGER("MailingList::isUserSender()");
  ASSERT(src);
  ASSERT(rec);
  ASSERT(lnam);

  // define storage type
  if (!strcasecmp(src, USE_LDAP_STORAGE))
  {
    stor = new LDAPStorage(STORE_SENDER, MODE_READ);
    src = group?group:lnam;
  }
  else
  {
    if (!strcasecmp(src, USE_DB_STORAGE))
    {
      stor = new DBStorage(STORE_SENDER, MODE_READ);
      src = lnam;
    }
    else
      stor = new FileStorage(STORE_SENDER, MODE_READ);
  }
  ASSERT(stor);

  adr = new Address;
  ASSERT(adr);
  adr->setUserName(rec->getUser());
  adr->setDomainName(rec->getDomain());

  if (stor->open(src) == TRUE)
  {
    block.add(adr);
    found = stor->find(&block);
    stor->close();
  }
  else
    mllog.add(1,"error: reading sender storage '%s'", src);

  delete stor;
  mllog.add(4,"info: user '%s@%s' %s sender of list '%s'",
              rec->getUser(), rec->getDomain(), found == TRUE?"is":"is NOT", lnam);
  return(found);
}

// adding user to list
BOOL MailingList::addUser(Address * rec)
{
  SBYTE * group = mlconfig.getValue(KEY_LIST_LDAP_USER_GROUP);
  SBYTE * src = mlconfig.getValue(KEY_LIST_USER_SOURCE);
  SBYTE * lnam = mlconfig.getValue(KEY_LIST_NAME);
  SBYTE * table = mlconfig.getValue(KEY_LIST_USERTABLE_NAME);
  Address * adr = NULL;
  Storage * stor = NULL;
  DataBlock block(TYPE_ADDRESS);
  BOOL flag = FALSE;

  MLDEBUGGER("MailingList::addUser()");
  ASSERT(src);
  ASSERT(rec);
  ASSERT(lnam);

  // define storage type
  if (!strcasecmp(src, USE_LDAP_STORAGE))
  {
    stor = new LDAPStorage(STORE_USER, MODE_APPEND);
    src = group?group:lnam;
  }
  else
  {
    if (!strcasecmp(src, USE_DB_STORAGE))
    {
      stor = new DBStorage(STORE_USER, MODE_APPEND);
      src = table?table:lnam;
    }
    else
      stor = new FileStorage(STORE_USER, MODE_APPEND);
  }

  ASSERT(stor);

  adr = new Address;
  ASSERT(adr);
  adr->setUserName(rec->getUser());
  adr->setDomainName(rec->getDomain());

  if (stor->open(src) == TRUE)
  {
    block.add(adr);
    if (stor->add(&block) == TRUE)
    {
      mllog.add(4,"info: user '%s@%s' added to list '%s'", rec->getUser(), rec->getDomain(), lnam);
      flag = TRUE;
    }
    else
      mllog.add(1,"error: can't write into storage '%s'", src);

    stor->close();
  }
  else
    mllog.add(1,"error: cannot open storage '%s'", src);

  delete stor;
  return(flag);
}

// remove user from list
BOOL MailingList::delUser(Address * rec)
{
  SBYTE * group = mlconfig.getValue(KEY_LIST_LDAP_USER_GROUP);
  SBYTE * src = mlconfig.getValue(KEY_LIST_USER_SOURCE);
  SBYTE * lnam = mlconfig.getValue(KEY_LIST_NAME);
  SBYTE * table = mlconfig.getValue(KEY_LIST_USERTABLE_NAME);
  Address * adr = NULL;
  Storage * stor = NULL;
  DataBlock block(TYPE_ADDRESS);
  BOOL flag = FALSE;

  MLDEBUGGER("MailingList::delUser()");
  ASSERT(src);
  ASSERT(rec);
  ASSERT(lnam);

  // define storage type
  if (!strcasecmp(src, USE_LDAP_STORAGE))
  {
    stor = new LDAPStorage(STORE_USER, MODE_DELETE);
    src = group?group:lnam;
  }
  else
  {
    if (!strcasecmp(src, USE_DB_STORAGE))
    {
      stor = new DBStorage(STORE_USER, MODE_DELETE);
      src = table?table:lnam;
    }
    else
      stor = new FileStorage(STORE_USER, MODE_READ);
  }
  ASSERT(stor);

  adr = new Address;
  ASSERT(adr);
  adr->setUserName(rec->getUser());
  adr->setDomainName(rec->getDomain());

  if (stor->open(src) == TRUE)
  {
    block.add(adr);
    if (stor->remove(&block) == TRUE)
    {
      mllog.add(4,"info: user '%s@%s' removed from list '%s'",
                  rec->getUser(), rec->getDomain(), lnam);
      flag = TRUE;
    }
    else
      mllog.add(1,"error: can't remove user '%s@%s' from list '%s'",
                  rec->getUser(), rec->getDomain(), lnam);

    stor->close();
  }
  else
    mllog.add(1,"error: opening user storage '%s'", src);

  delete stor;
  return(flag);
}

// remove sender from list
BOOL MailingList::delSender(Address * rec)
{
  SBYTE * group = mlconfig.getValue(KEY_LIST_LDAP_SENDER_GROUP);
  SBYTE * src = mlconfig.getValue(KEY_LIST_SENDER_SOURCE);
  SBYTE * lnam = mlconfig.getValue(KEY_LIST_NAME);
  Address * adr = NULL;
  Storage * stor = NULL;
  DataBlock block(TYPE_ADDRESS);
  BOOL flag = FALSE;

  MLDEBUGGER("MailingList::delSender()");
  ASSERT(src);
  ASSERT(lnam);
  ASSERT(rec);

  // define storage type
  if (!strcasecmp(src, USE_LDAP_STORAGE))
  {
    stor = new LDAPStorage(STORE_SENDER, MODE_DELETE);
    src = group?group:lnam;
  }
  else
  {
    if (!strcasecmp(src, USE_DB_STORAGE))
    {
      stor = new DBStorage(STORE_SENDER, MODE_DELETE);
      src = lnam;
    }
    else
      stor = new FileStorage(STORE_SENDER, MODE_READ);
  }
  ASSERT(stor);

  adr = new Address;
  ASSERT(adr);
  adr->setUserName(rec->getUser());
  adr->setDomainName(rec->getDomain());

  if (stor->open(src) == TRUE)
  {
    block.add(adr);
    if (stor->remove(&block) == TRUE)
    {
      mllog.add(4,"info: sender '%s@%s' removed from list '%s'",
                  rec->getUser(), rec->getDomain(), lnam);
      flag = TRUE;
    }
    else
      mllog.add(1,"error: can't remove sender '%s@%s' from list '%s'",
                  rec->getUser(), rec->getDomain(), lnam);

    stor->close();
  }
  else
    mllog.add(1,"error: opening user storage '%s'", src);

  delete stor;
  return(flag);
}

// adding sender to list
BOOL MailingList::addSender(Address * rec)
{
  SBYTE * group = mlconfig.getValue(KEY_LIST_LDAP_SENDER_GROUP);
  SBYTE * src = mlconfig.getValue(KEY_LIST_SENDER_SOURCE);
  SBYTE * lnam = mlconfig.getValue(KEY_LIST_NAME);
  Address * adr = NULL;
  Storage * stor = NULL;
  DataBlock block(TYPE_ADDRESS);
  BOOL flag = FALSE;

  MLDEBUGGER("MailingList::addSender()");
  ASSERT(src);
  ASSERT(lnam);
  ASSERT(rec);

  // define storage type
  if (!strcasecmp(src, USE_LDAP_STORAGE))
  {
    stor = new LDAPStorage(STORE_SENDER, MODE_APPEND);
    src = group?group:lnam;
  }
  else
  {
    if (!strcasecmp(src, USE_DB_STORAGE))
    {
      stor = new DBStorage(STORE_SENDER, MODE_APPEND);
      src = lnam;
    }
    else
      stor = new FileStorage(STORE_SENDER, MODE_APPEND);
  }
  ASSERT(stor);

  adr = new Address;
  ASSERT(adr);
  adr->setUserName(rec->getUser());
  adr->setDomainName(rec->getDomain());

  if (stor->open(src) == TRUE)
  {
    block.add(adr);
    if (stor->add(&block) == TRUE)
    {
      mllog.add(4,"info: sender '%s@%s' added to list '%s'", rec->getUser(), rec->getDomain(), lnam);
      flag = TRUE;
    }
    else
      mllog.add(1,"error: can't write into storage '%s'", src);

    stor->close();
  }
  else
    mllog.add(1,"error: cannot open storage '%s'", src);

  delete stor;
  return(flag);
}

// return number of subscribed users
ULONG MailingList::getUserCount(VOID)
{
  SBYTE * group = mlconfig.getValue(KEY_LIST_LDAP_USER_GROUP);
  SBYTE * src = mlconfig.getValue(KEY_LIST_USER_SOURCE);
  SBYTE * lnam = mlconfig.getValue(KEY_LIST_NAME);
  SBYTE * table = mlconfig.getValue(KEY_LIST_USERTABLE_NAME);

  Storage * stor = NULL;
  ULONG num = 0;

  MLDEBUGGER("MailingList::getUserCount()");
  ASSERT(src);
  ASSERT(lnam);

  // define storage type
  if (!strcasecmp(src, USE_LDAP_STORAGE))
  {
    stor = new LDAPStorage(STORE_USER, MODE_READ);
    src = group?group:lnam;
  }
  else
  {
    if (!strcasecmp(src, USE_DB_STORAGE))
    {
      stor = new DBStorage(STORE_USER, MODE_READ);
      src = table?table:lnam;
    }
    else
      stor = new FileStorage(STORE_USER, MODE_READ);
  }
  ASSERT(stor);

  if (stor->open(src) == TRUE)
  {
    num = stor->getCount();
    stor->close();
  }
  else
    mllog.add(1,"error: cannot open storage '%s'", src);

  delete stor;
  return(num);
}

// check if given user is allowed to broadcast
BOOL MailingList::isUserAuthorized(Address * usr)
{
  SBYTE * policy = mlconfig.getValue(KEY_LIST_SEND);
  BOOL found = FALSE, ok = FALSE;

  MLDEBUGGER("MailingList::isUserAuthorized()");
  ASSERT(usr);

  if (!policy)
  {
    mllog.add(3,"warning: 'list_send' not defined, assuming 'closed', rejected");
    return(FALSE);
  }

  // closed - never broadcast
  if (!found && !strcasecmp(policy, "closed"))
  {
    found = TRUE;
    ok = FALSE;
  }

  // open - everyone can send to the list
  if (!found && !strcasecmp(policy, "open"))
  {
    found = TRUE;
    ok = TRUE;
  }

  // subscriber - only subscribed people
  if (!found && !strcasecmp(policy, "subscriber"))
  {
    found = TRUE;
    ok = isUserMember(usr);
  }

  // sender - only people who are senders
  if (!found && !strcasecmp(policy, "sender"))
  {
    found = TRUE;
    ok = isUserSender(usr);
  }

  if (!found)
  {
    mllog.add(3,"warning: invalid broadcast policy '%s', mail from '%s@%s' rejected",
                policy, usr->getUser(), usr->getDomain());
  }
  else
  {
    mllog.add(3,"info: broadcast policy '%s', mail from '%s@%s' %s",
                policy, usr->getUser(), usr->getDomain(), ok?"accepted":"rejected");
  }
  return(ok);
}
