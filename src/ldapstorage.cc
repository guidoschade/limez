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

#include "ldapstorage_def.h"
#include "lightdir_def.h"
#include "log_def.h"
#include "config_def.h"

#include "line_def.h"
#include "address_inl.h"
#include "keyvalue_inl.h"
#include "datablock_inl.h"

extern Log log;
extern Config config;
extern Timer timer;

SBYTE * NO_LDAP_ERR = {"error: ldap support not configured, please recompile"};

LDAPStorage::LDAPStorage(STORECLASS sclass, FILEMODE fmode)
{
#ifdef USE_LDAP
  this->fmode = fmode;
  this->sclass = sclass;
  opened = FALSE;
  lite = NULL;
#endif
}

LDAPStorage::~LDAPStorage()
{
#ifdef USE_LDAP
  ASSERT(!opened);
  ASSERT(!lite);
#endif
}

// try to open connection to server
BOOL LDAPStorage::open(const SBYTE * group)
{
#ifdef USE_LDAP
  DEBUGGER("LDAPStorage::open()");
  ASSERT(!opened);
  ASSERT(group);

  strncpy(name, group, MAX_VAL_SIZE);

  lite = new LightDir;
  ASSERT(lite);

  // connecting to ldap
  if (lite->connect(config.getValue(KEY_LDAP_HOSTS),
      config.getValue(KEY_LDAP_USER), config.getValue(KEY_LDAP_PASS)) == FALSE)
  {
    delete lite;
    lite = NULL;
    log.add(1,"error: connect to ldap-server failed");
    return(FALSE);
  }
  opened = TRUE;
  return(TRUE);

#else
  log.add(1, NO_LDAP_ERR);
  return(FALSE);
#endif
}

// close connection to database
BOOL LDAPStorage::close(VOID)
{
#ifdef USE_LDAP
  DEBUGGER("LDAPStorage::close()");
  ASSERT(opened);
  ASSERT(lite);

  lite->clearResults();
  lite->disconnect();
  delete lite;
  lite = NULL;
  opened = FALSE;
  return(TRUE);

#else
  log.add(1, NO_LDAP_ERR);
  return(FALSE);
#endif
}

// add data to ldap
BOOL LDAPStorage::add(DataBlock * block)
{
#ifdef USE_LDAP
  DEBUGGER("LDAPStorage::add()");
  ASSERT(block);
  ASSERT(opened);
  ASSERT(fmode == MODE_WRITE || fmode == MODE_APPEND);

  log.add(1,"error: LDAPStorage::add() not implemented");
  return(FALSE);

#else
  log.add(1, NO_LDAP_ERR);
  return(FALSE);
#endif
}

// get data from ldap-server
BOOL LDAPStorage::get(DataBlock * block, ULONG start = 1, ULONG count = 0)
{
#ifdef USE_LDAP
  DEBUGGER("LDAPStorage::get()");

  ULONG current = 0;
  ULONG end = (start + count)-1;

  SBYTE * base = config.getValue(KEY_LDAP_BASE_DN);
  SBYTE * gfilter = config.getValue(KEY_LDAP_FILTER);
  SBYTE * ufilter = "(objectclass=*)";

  SBYTE * gattrs[2] = {"uniquemember", NULL};
  SBYTE * uattrs[4] = {"givenname", "sn", "mail", NULL};

  SBYTE   filt[MAX_LDAP_FILTER_SIZE+1];
  KeyValue ** curr = NULL;
  KeyValue * tmp = NULL;

  List<KeyValue> values;

  ASSERT(opened);
  ASSERT(fmode == MODE_READ);
  ASSERT(block);

  BOOL got = FALSE;

  switch(sclass)
  {
    case(STORE_SENDER):
    case(STORE_USER):
    {
      SBYTE * mail = NULL;
      Address * adr = NULL;
      SBYTE * ret = NULL;

      DEBUGGER("LDAPStorage::get():STORE_USER/SENDER");
      ASSERT(block->getType() == TYPE_ADDRESS);

      snprintf(filt, MAX_LDAP_FILTER_SIZE, "(&(cn=%s)%s)", name,
               gfilter?gfilter:"(objectclass=groupofuniquenames)(objectclass=mailgroup)");

      log.add(4,"info: ldap searching (base: %s, filter: %s)", base, filt);

      // searching users belonging to the given group
      if (lite->search(base, TRUE, filt, gattrs) == FALSE)
      {
        log.add(3,"error: ldap search (base: %s, filter: %s) failed", base, filt);
        return(FALSE);
      }

      // getting user DNs
      while((curr = lite->getEntry()))
        for (INT x = 0; curr[x]; x++)
          values.add(curr[x]);

      lite->clearResults();
      log.add(4,"info: search returned %ld values", values.getCount());

      // get users, ignore DN (first value)
      tmp = values.getFirst();
      while ((tmp = values.getNext(tmp)))
      {
        log.add(6,"ldap search: '%s' = '%s'", tmp->getKey(), tmp->getVal());

        // only return wanted values
        if (++current >= start)
        {
          // searching for user
          if (lite->search(tmp->getVal(), FALSE, ufilter, uattrs) == FALSE)
          {
            log.add(3,"error: ldap search (base: %s, filter: %s) failed", tmp->getVal(), ufilter);
            continue;
          }

          // getting user attributes, should only be one entry
          curr = lite->getEntry();

          // get the e-mail-entry
          for (INT x = 0; curr[x]; x++)
            if (!strcmp("mail", curr[x]->getKey()))
              mail = curr[x]->getVal();

          // be sure we got an e-mail address
          if (!mail)
          {
            log.add(3,"error: entry does not contain mail attribute");
            lite->clearResults();
            continue;
          }

          adr = new Address;
          ASSERT(adr);

          if ((ret = adr->setIfValid(mail, FALSE)))
          {
            log.add(2,"error: '%s', address: %s", ret, mail);
            delete adr;
          }
          else
          {
            log.add(7,"info: read address '%s@%s'", adr->getUser(), adr->getDomain());
            got = TRUE;
            block->add(adr);
          }

          // free buffer & clear results
          curr = lite->getEntry();
          ASSERT(!curr);
          lite->clearResults();
        }
        if (count && (current >= end))
          break;
      }

      values.clear();
      return(got);
      break;
    }

    default:
      log.add(1,"error: LDAPStorage::get()XXX not implemented");
      return(FALSE);
      break;
  }
  return(FALSE);

#else
  log.add(1, NO_LDAP_ERR);
  return(FALSE);
#endif
}

// delete some entries
BOOL LDAPStorage::remove(DataBlock * block)
{
#ifdef USE_LDAP
  DEBUGGER("LDAPStorage::remove()");
  ASSERT(opened);
  ASSERT(fmode == MODE_DELETE);
  ASSERT(block);

  log.add(1,"error: LDAPStorage::remove() not implemented");
  return(FALSE);

#else
  log.add(1, NO_LDAP_ERR);
  return(FALSE);
#endif
}

// change entries in ldap
BOOL LDAPStorage::change(DataBlock * block)
{
#ifdef USE_LDAP
  DEBUGGER("LDAPStorage::change()");
  ASSERT(opened);
  ASSERT(fmode == MODE_READ);
  ASSERT(block);

  log.add(1,"error: LDAPStorage::change() not implemented");
  return(FALSE);

#else
  log.add(1, NO_LDAP_ERR);
  return(FALSE);
#endif
}

// drop vall values from table
BOOL LDAPStorage::destroy(SBYTE * nam)
{
#ifdef USE_LDAP
  DEBUGGER("LDAPStorage::destroy()");
  ASSERT(opened);

  log.add(1,"error: LDAPStorage::destroy() not implemented");
  return(FALSE);

#else
  log.add(1, NO_LDAP_ERR);
  return(FALSE);
#endif
}

// rename table
BOOL LDAPStorage::reName(const SBYTE * newname)
{
#ifdef USE_LDAP
  DEBUGGER("LDAPStorage::reName()");
  ASSERT(newname);

  log.add(1,"error: LDAPStorage::reName() not implemented");
  return(FALSE);

#else
  log.add(1, NO_LDAP_ERR);
  return(FALSE);
#endif
}

// getNumber
ULONG LDAPStorage::getCount(VOID)
{
#ifdef USE_LDAP
  DEBUGGER("LDAPStorage::getCount()");
  ASSERT(opened);
  ASSERT(fmode == MODE_READ);

  ULONG num = 0;
  SBYTE * base = config.getValue(KEY_LDAP_BASE_DN);
  SBYTE * gfilter = config.getValue(KEY_LDAP_FILTER);
  SBYTE * gattrs[2] = {"uniquemember", NULL};

  SBYTE   filt[MAX_LDAP_FILTER_SIZE+1];
  KeyValue ** curr = NULL;

  switch(sclass)
  {
    case(STORE_SENDER):
    case(STORE_USER):
    {
      DEBUGGER("LDAPStorage::getCount(STORE_USER/SENDER)");

      snprintf(filt, MAX_LDAP_FILTER_SIZE, "(&(cn=%s)%s)", name,
               gfilter?gfilter:"(objectclass=groupofuniquenames)(objectclass=mailgroup)");

      log.add(5,"info: ldap searching (base: %s, filter: %s)", base, filt);

      // searching users belonging to the given group
      if (lite->search(base, TRUE, filt, gattrs) == FALSE)
      {
        log.add(3,"error: ldap search (base: %s, filter: %s) failed", base, filt);
        return(FALSE);
      }

      // getting user DNs
      while((curr = lite->getEntry()))
        for (INT x = 0; curr[x]; x++)
          num ++;

      lite->clearResults();
      num -= 1;

      log.add(5,"info: getCount returned %ld values", num);
      return(num);
      break;
    }
    default:
      log.add(1,"error: LDAPStorage::getCount()XXX not implemented");
      return(FALSE);
      break;
  }
  return(FALSE);

#else
  log.add(1, NO_LDAP_ERR);
  return(FALSE);
#endif
}

// find entry in database
BOOL LDAPStorage::find(DataBlock * block)
{
#ifdef USE_LDAP
  DEBUGGER("LDAPStorage::find()");

  ASSERT(opened);
  ASSERT(fmode == MODE_READ);
  ASSERT(block);

  BOOL got = FALSE;
  SBYTE filt[MAX_LDAP_FILTER_SIZE+1];
  SBYTE userdn[MAX_VAL_SIZE+1];

  SBYTE * uattrs[2] = {"mail", NULL};
  SBYTE * gattrs[2] = {"uniquemember", NULL};
  SBYTE * base = config.getValue(KEY_LDAP_BASE_DN);
  SBYTE * gfilter = config.getValue(KEY_LDAP_FILTER);

  KeyValue ** curr = NULL;

  switch(sclass)
  {
    case(STORE_SENDER):
    case(STORE_USER):
    {
      Address * tmp  = NULL;
      DEBUGGER("LDAPStorage::find(STORE_USER/SENDER)");

      if (block->get(&tmp) == TRUE)
      {
        ASSERT(tmp);

        // search distinguished name where the e-mail matches
        snprintf(filt, MAX_LDAP_FILTER_SIZE, "(mail=%s@%s)", tmp->getUser(), tmp->getDomain());

        log.add(5,"info: ldap search (base: %s, filter: %s)", base, filt);

        // searching users belonging to the given group
        if (lite->search(base, TRUE, filt, uattrs) == FALSE)
        {
          log.add(3,"error: ldap search (base: %s, filter: %s) failed", base, filt);
          return(FALSE);
        }

        // getting user DN we are searching for
        while((curr = lite->getEntry()))
          for (INT x = 0; curr[x]; x++)
            if (!strcmp(curr[x]->getKey(), "dn"))
              snprintf(userdn, MAX_VAL_SIZE, "%s", curr[x]->getVal());

        lite->clearResults();
        log.add(7,"info: user %s found ...", userdn);

        snprintf(filt, MAX_LDAP_FILTER_SIZE, "(&(cn=%s)%s)", name,
               gfilter?gfilter:"(objectclass=groupofuniquenames)(objectclass=mailgroup)");

        // get all users from group
        if (lite->search(base, TRUE, filt, gattrs) == FALSE)
        {
          log.add(3,"error: ldap search (base: %s, filter: %s) failed", base, filt);
          return(FALSE);
        }

        // getting all users in group
        while((curr = lite->getEntry()))
          for (INT x = 0; curr[x]; x++)
            if (!strcmp(curr[x]->getVal(), userdn))
              got = TRUE;

        lite->clearResults();
      }
      return(got);
      break;
    }
    default:
      log.add(1,"error: LDAPStorage::find()XXX not implemented");
      return(FALSE);
      break;
  }
  return(FALSE);

#else
  log.add(1, NO_LDAP_ERR);
  return(FALSE);
#endif
}

// copy contents of other storage to current
BOOL LDAPStorage::clone(const SBYTE * newname)
{
#ifdef USE_LDAP
  DEBUGGER("LDAPStorage::clone()");
  ASSERT(opened);
  ASSERT(newname);

  switch(sclass)
  {
    case(STORE_USER):
    {
    }
    default: break;
  }
  log.add(1,"error: LDAPStorage::clone() not implemented");
  return(FALSE);

#else
  log.add(1, NO_LDAP_ERR);
  return(FALSE);
#endif
}
