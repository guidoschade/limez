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

#include "config_def.h"

#include "log_def.h"
#include "list_def.h"

#include "log_inl.h"
#include "list_inl.h"
#include "keyvalue_inl.h"
#include "datablock_inl.h"
#include "connection_inl.h"

extern Log log;
extern Config  config;

Config::Config(SBYTE * n)
{
  name = n;
}

Config::~Config()
{
}

// every config needs a name, can be changed here
VOID Config::setName(SBYTE * n)
{
  name = n;
}

// set keys if config should be limez main conf
VOID Config::initMainKeys(VOID)
{
  DEBUGGER("Config::initMainKeys()");
  keys.clear();

  // setting valid configuration keys, types, options and default values
#ifdef USE_DATABASE
  keys.add(new KeyValue(KEY_DB_HOST, TYPE_STR, CONF_UNIQ + CONF_NOEDIT));
  keys.add(new KeyValue(KEY_DB_NAME, TYPE_STR, CONF_UNIQ + CONF_NOEDIT));
  keys.add(new KeyValue(KEY_DB_USER, TYPE_STR, CONF_UNIQ + CONF_NOEDIT));
  keys.add(new KeyValue(KEY_DB_PASS, TYPE_STR, CONF_UNIQ + CONF_NOEDIT));
  keys.add(new KeyValue(KEY_MAILINGLIST_DB, TYPE_STR, CONF_MULT + CONF_NOEDIT));
#endif

#ifdef USE_LDAP
  keys.add(new KeyValue(KEY_LDAP_HOSTS, TYPE_STR, CONF_UNIQ));
  keys.add(new KeyValue(KEY_LDAP_USER, TYPE_STR, CONF_UNIQ));
  keys.add(new KeyValue(KEY_LDAP_PASS, TYPE_STR, CONF_UNIQ));
  keys.add(new KeyValue(KEY_LDAP_BASE_DN, TYPE_STR, CONF_UNIQ));
  keys.add(new KeyValue(KEY_LDAP_FILTER, TYPE_STR, CONF_UNIQ));
#endif

  keys.add(new KeyValue(KEY_LOGFILE, TYPE_STR, CONF_UNIQ + CONF_MAND, "limez.log"));
  keys.add(new KeyValue(KEY_DEBUG_LEVEL, TYPE_XINT, CONF_UNIQ + CONF_MAND, "3"));
  keys.add(new KeyValue(KEY_DEBUG_TEST_ONLY, TYPE_BOOL, CONF_UNIQ + CONF_TEMP));
  keys.add(new KeyValue(KEY_DEBUG_CONNECT_ONLY, TYPE_BOOL, CONF_UNIQ + CONF_TEMP));
  keys.add(new KeyValue(KEY_MAILINGLIST, TYPE_STR, CONF_MULT + CONF_NOEDIT));

  keys.add(new KeyValue(KEY_SERVER_ROOT, TYPE_STR, CONF_UNIQ));
  keys.add(new KeyValue(KEY_SERVER_DAEMONIZE, TYPE_BOOL, CONF_UNIQ + CONF_TEMP));
  keys.add(new KeyValue(KEY_SERVER_SPOOL_ONLY, TYPE_XINT, CONF_UNIQ));
  keys.add(new KeyValue(KEY_SERVER_SEND_HOST, TYPE_STR, CONF_UNIQ));
  keys.add(new KeyValue(KEY_SERVER_BACKLOG, TYPE_XINT, CONF_UNIQ + CONF_MAND, "5"));
  keys.add(new KeyValue(KEY_SERVER_ADMIN, TYPE_STR, CONF_UNIQ + CONF_MAND, "admin@limez.net"));
  keys.add(new KeyValue(KEY_SERVER_ALIAS, TYPE_STR, CONF_MULT));
  keys.add(new KeyValue(KEY_SERVER_HOST, TYPE_STR, CONF_UNIQ));
  keys.add(new KeyValue(KEY_SERVER_PORT, TYPE_XINT, CONF_UNIQ + CONF_MAND + CONF_NOEDIT, "25"));
  keys.add(new KeyValue(KEY_SERVER_REMOTE_PORT, TYPE_XINT, CONF_UNIQ + CONF_MAND, "25"));
  keys.add(new KeyValue(KEY_SERVER_TIMEOUT, TYPE_XINT, CONF_UNIQ + CONF_MAND, "120"));
  keys.add(new KeyValue(KEY_SERVER_MAIL, TYPE_STR, CONF_UNIQ + CONF_MAND, "limez"));
  keys.add(new KeyValue(KEY_SERVER_DOMAIN, TYPE_STR, CONF_UNIQ + CONF_MAND, "limez.net"));
  keys.add(new KeyValue(KEY_SERVER_UID, TYPE_STR, CONF_UNIQ + CONF_NOEDIT));
  keys.add(new KeyValue(KEY_SERVER_GID, TYPE_STR, CONF_UNIQ + CONF_NOEDIT));
  keys.add(new KeyValue(KEY_SERVER_STRING, TYPE_STR, CONF_UNIQ + CONF_MAND, "List Mezzenger"));
  keys.add(new KeyValue(KEY_SERVER_SUBJECT, TYPE_STR, CONF_UNIQ + CONF_MAND, "Limez results"));
  keys.add(new KeyValue(KEY_SERVER_CONFIG, TYPE_STR, CONF_UNIQ + CONF_MAND + CONF_NOEDIT, "limez.config"));
  keys.add(new KeyValue(KEY_SERVER_MAX_PROCS, TYPE_XINT, CONF_UNIQ + CONF_MAND, "16"));
  keys.add(new KeyValue(KEY_SERVER_CACHETIME, TYPE_XINT, CONF_UNIQ + CONF_MAND, "86400"));
  keys.add(new KeyValue(KEY_SERVER_MAX_HOPS, TYPE_XINT, CONF_UNIQ + CONF_MAND, "10"));
  keys.add(new KeyValue(KEY_SERVER_WEB_PASS, TYPE_STR, CONF_UNIQ + CONF_MAND + CONF_NOEDIT, "admin"));
  keys.add(new KeyValue(KEY_SERVER_WEB_HOSTS, TYPE_STR, CONF_MULT + CONF_MAND, "127.0.0.1"));
  keys.add(new KeyValue(KEY_SERVER_QUEUE_INTERVAL, TYPE_XINT, CONF_UNIQ + CONF_MAND, "1800"));
  keys.add(new KeyValue(KEY_SERVER_SPOOL_INTERVAL, TYPE_XINT, CONF_UNIQ + CONF_MAND, "10"));
  keys.add(new KeyValue(KEY_SERVER_MAX_QUEUETIME, TYPE_XINT, CONF_UNIQ + CONF_MAND, "432000"));
  keys.add(new KeyValue(KEY_SERVER_MAX_QUEUETRIES, TYPE_XINT, CONF_UNIQ + CONF_MAND, "100"));
  keys.add(new KeyValue(KEY_SERVER_MAX_MAILSIZE, TYPE_XINT, CONF_UNIQ + CONF_MAND, "3145728"));
  keys.add(new KeyValue(KEY_SERVER_MAX_SENDPROCS, TYPE_XINT, CONF_UNIQ));
  keys.add(new KeyValue(KEY_SERVER_SPOOL_DIR, TYPE_STR, CONF_UNIQ + CONF_MAND, "spool/"));
  keys.add(new KeyValue(KEY_SERVER_DONE_DIR, TYPE_STR, CONF_UNIQ + CONF_MAND, "done/"));
  keys.add(new KeyValue(KEY_SERVER_FAKE_NAME, TYPE_STR, CONF_UNIQ));
  keys.add(new KeyValue(KEY_SERVER_COMMAND_QUIET, TYPE_BOOL, CONF_UNIQ));
}

// listconfig
VOID Config::initListKeys(VOID)
{
  DEBUGGER("Config::initListKeys()");
  keys.clear();

  keys.add(new KeyValue(KEY_LOGFILE, TYPE_STR, CONF_UNIQ + CONF_MAND, config.getValue(KEY_LOGFILE)));
  keys.add(new KeyValue(KEY_DEBUG_LEVEL, TYPE_XINT, CONF_UNIQ + CONF_MAND, config.getValue(KEY_DEBUG_LEVEL)));

  keys.add(new KeyValue(KEY_LIST_MAX_SEND_TASKS, TYPE_XINT, CONF_UNIQ + CONF_MAND, "16"));
  keys.add(new KeyValue(KEY_LIST_MAX_MAILSIZE, TYPE_XINT, CONF_UNIQ + CONF_MAND, "40960"));
  keys.add(new KeyValue(KEY_LIST_NAME, TYPE_STR, CONF_UNIQ + CONF_MAND + CONF_NOEDIT));
  keys.add(new KeyValue(KEY_LIST_PASS));
  keys.add(new KeyValue(KEY_LIST_CONFIG, TYPE_STR, CONF_UNIQ + CONF_MAND + CONF_NOEDIT, "database"));
  keys.add(new KeyValue(KEY_LIST_SEND, TYPE_STR, CONF_UNIQ + CONF_MAND, "subscriber"));
  keys.add(new KeyValue(KEY_LIST_DESCR));
  keys.add(new KeyValue(KEY_LIST_SUBSCRIBE, TYPE_STR, CONF_UNIQ + CONF_MAND, "open"));
  keys.add(new KeyValue(KEY_LIST_UNSUBSCRIBE, TYPE_STR, CONF_UNIQ + CONF_MAND, "open"));
  keys.add(new KeyValue(KEY_LIST_STAT, TYPE_STR, CONF_UNIQ + CONF_MAND, "closed"));
  keys.add(new KeyValue(KEY_LIST_INFO, TYPE_STR, CONF_UNIQ + CONF_MAND, "closed"));
  keys.add(new KeyValue(KEY_LIST_ADMIN, TYPE_STR, CONF_UNIQ + CONF_MAND, config.getValue(KEY_SERVER_ADMIN)));
  keys.add(new KeyValue(KEY_LIST_DOMAIN, TYPE_STR, CONF_UNIQ + CONF_MAND, config.getValue(KEY_SERVER_DOMAIN)));
  keys.add(new KeyValue(KEY_LIST_USER_SOURCE, TYPE_STR, CONF_UNIQ + CONF_MAND, "database"));
  keys.add(new KeyValue(KEY_LIST_SENDER_SOURCE, TYPE_STR, CONF_UNIQ + CONF_MAND, "database"));
  keys.add(new KeyValue(KEY_LIST_DISABLE_SPOOL, TYPE_BOOL));
  keys.add(new KeyValue(KEY_LIST_WELCOME));
  keys.add(new KeyValue(KEY_LIST_ME_TOO, TYPE_BOOL));
  keys.add(new KeyValue(KEY_LIST_INFORM_ADMIN, TYPE_BOOL));
  keys.add(new KeyValue(KEY_LIST_GOODBYE));
  keys.add(new KeyValue(KEY_LIST_VERBOSE, TYPE_BOOL));
  keys.add(new KeyValue(KEY_LIST_HEADER_FROM));
  keys.add(new KeyValue(KEY_LIST_HEADER_RECIPIENT));
  keys.add(new KeyValue(KEY_LIST_HEADER_SENDER));
  keys.add(new KeyValue(KEY_LIST_AUTO_UNSUB, TYPE_BOOL));
  keys.add(new KeyValue(KEY_LIST_HEADER_REPLY));
  keys.add(new KeyValue(KEY_LIST_HEADER_SUBJECT));
  keys.add(new KeyValue(KEY_LIST_HEADER_FILE));
  keys.add(new KeyValue(KEY_LIST_FOOTER_FILE));
  keys.add(new KeyValue(KEY_LIST_LDAP_USER_GROUP));
  keys.add(new KeyValue(KEY_LIST_LDAP_SENDER_GROUP));

#ifdef USE_DATABASE
  keys.add(new KeyValue(KEY_LIST_USERTABLE_NAME));
#endif
}

// config of message
VOID Config::initMsgKeys(VOID)
{
  DEBUGGER("Config::initMsgKeys()");
  keys.clear();

  keys.add(new KeyValue(KEY_MESG_RCPT, TYPE_STR, CONF_MULT + CONF_MAND));
  keys.add(new KeyValue(KEY_MESG_FROM, TYPE_STR, CONF_UNIQ + CONF_MAND));
  keys.add(new KeyValue(KEY_MESG_STAT, TYPE_STR, CONF_UNIQ + CONF_MAND));
  keys.add(new KeyValue(KEY_MESG_NAME, TYPE_STR, CONF_UNIQ + CONF_MAND));
  keys.add(new KeyValue(KEY_MESG_TRIES, TYPE_XINT, CONF_UNIQ + CONF_MAND));
  keys.add(new KeyValue(KEY_MESG_SIZE, TYPE_XINT, CONF_UNIQ + CONF_MAND));
  keys.add(new KeyValue(KEY_MESG_SEND, TYPE_STR, CONF_UNIQ + CONF_MAND));
  keys.add(new KeyValue(KEY_MESG_INIT_DATE, TYPE_XINT, CONF_UNIQ + CONF_MAND));
  keys.add(new KeyValue(KEY_MESG_CHNG_DATE, TYPE_XINT, CONF_UNIQ + CONF_MAND));
  keys.add(new KeyValue(KEY_MESG_ORIG, TYPE_STR, CONF_UNIQ));
  keys.add(new KeyValue(KEY_MESG_LIST_NAME, TYPE_STR, CONF_UNIQ));
}

// lock - file
VOID Config::initLockKeys(VOID)
{
  DEBUGGER("Config::initLockKeys()");
  keys.clear();

  keys.add(new KeyValue(KEY_LOCK_PID, TYPE_XINT, CONF_UNIQ + CONF_MAND));
  keys.add(new KeyValue(KEY_LOCK_DATE, TYPE_XINT, CONF_UNIQ + CONF_MAND));
}

// inserting new value into list
BOOL Config::addValue(const SBYTE * key, const SBYTE * value = NULL, CONFOPT opts = CONF_NONE)
{
  KeyValue * currKey = NULL, * currValue = NULL;

  DEBUGGER("Config::addValue()");
  ASSERT(key);

  if (!value)
  {
    log.add(3,"warning: value for key '%s' is NULL pointer, not set", key);
    return(FALSE);
  }

  if (strlen(value) < 1)
  {
    log.add(3,"warning: value for key '%s' is empty, not set", key);
    return(FALSE);
  }

  // look if given key is known
  currKey = keys.getFirst();
  while(currKey)
  {
    if (!strcmp(currKey->getKey(), key))
    {
      // get value containing key
      currValue = values.getFirst();
      while (currValue)
      {
        if (!strcmp(currValue->getKey(), key))
        {
           // is this key unique
           if (!(currKey->options & CONF_MULT))
           {
             // is this key protected
             if (currValue->options & CONF_PROT)
             {
               log.add(7,"info: key '%s' protected, not changed", key);
               return(TRUE);
             }
             else
             {
               log.add(7,"info: config, overwriting key '%s'", key);
               values.del(currValue);
               delete currValue;
               break;
             }
           }
           else
           {
             if (!strcmp(currValue->getVal(), value))
             {
               log.add(7,"info: config, multiple key '%s' with value '%s' already set", key, value);
               return(TRUE);
             }
           }
        }
        currValue = values.getNext(currValue);
      }

      // adding new value to config
      currValue = new KeyValue(key, currKey->vtype, currKey->options | opts, value);
      ASSERT(currValue);
      values.add(currValue);

      return(TRUE);
    }
    currKey = keys.getNext(currKey);
  }
  log.add(1,"error: tried to add unknown key '%s' = '%s' into config", key, value);
  return(FALSE);
}

// setting default values
VOID Config::setDefaults()
{
  KeyValue * currKey = NULL, * currValue = NULL;

  DEBUGGER("Config::setDefaults()");

  // set options and type for already existing entries
  currValue = values.getFirst();
  while(currValue)
  {
    currKey = keys.getFirst();
    while(currKey)
    {
      if (!strcmp(currValue->getKey(), currKey->getKey()))
      {
        currValue->options = currValue->options | currKey->options;
        currValue->vtype = currKey->vtype;
      }
      currKey = keys.getNext(currKey);
    }
    currValue = values.getNext(currValue);
  }

  currKey = keys.getFirst();
  while(currKey)
  {
    // only set mandatory values and only if set to any value
    if ((currKey->options & CONF_MAND) && (currKey->getVal() != NULL))
      addValue(currKey->getKey(), currKey->getVal());

    currKey = keys.getNext(currKey);
  }

  DEBUGGER("Config::setDefaults(end)");
}

// checking config
BOOL Config::check()
{
  BOOL found, err = FALSE;
  KeyValue * currKey = NULL, * currValue = NULL;

  DEBUGGER("Config::check()");

  // check all known keys
  currKey = keys.getFirst();
  while(currKey)
  {
    found = FALSE;

    // search if key is set in config
    currValue = values.getFirst();
    while(currValue)
    {
      if (!strcmp(currKey->getKey(), currValue->getKey()))
      {
        found = TRUE;
        break;
      }
      currValue = values.getNext(currValue);
    }

    // look if key is mandatory
    if ((currKey->options & CONF_MAND) && !found)
    {
      log.add(1,"error: no mandatory key '%s' in config", currKey->getKey());
      err = TRUE;
    }
    currKey = keys.getNext(currKey);
  }

  if (err == TRUE)
    return(FALSE);
  else
    return(TRUE);
}

// returning value of given key
SBYTE * Config::getValue(const SBYTE * key)
{
  KeyValue * currValue = NULL;

  DEBUGGER("Config::getValue()");
  ASSERT(key);

  log.add(5,"info: config::getValue('%s')", key);

  // search Value if we alredy have it
  currValue = values.getFirst();
  while (currValue)
  {
    if (!strcasecmp(currValue->getKey(), key))
    {
      if (currValue->vtype == TYPE_XINT && !currValue->getVal())
      {
        log.add(1,"error: config cannot return integer with value NULL");
        ASSERT(FALSE);
      }
      return(currValue->getVal());
    }
    currValue = values.getNext(currValue);
  }
  log.add(9,"info: config key '%s' not set", key);
  return(NULL);
}

// returning value (as long) of given key
ULONG Config::getIntValue(const SBYTE * key)
{
  SBYTE * v = NULL;
  ULONG x;

  DEBUGGER("Config::getIntValue()");
  ASSERT(key);

  if ((v = getValue(key)))
    x = atol(v);
  else
  {
    log.add(1,"error: config cannot return integer with value NULL");
    ASSERT(FALSE);
    x = 0;
  }
  return(x);
}

// returning value (as bool) of given key
BOOL Config::getBoolValue(const SBYTE * key)
{
  SBYTE * v = NULL;
  BOOL x = FALSE;

  DEBUGGER("Config::getBoolValue()");
  ASSERT(key);

  if ((v = getValue(key)))
  {
    if (!strcasecmp(v,"1") || !strcasecmp(v,"YES") ||
        !strcasecmp(v,"TRUE") || !strcasecmp(v,"ON"))
      x = TRUE;
  }

  return(x);
}

// get values of duplicate keys: 1 - first, 2 - second
SBYTE * Config::getNext(ULONG cnt, SBYTE * key)
{
  KeyValue * currValue = NULL;

  DEBUGGER("Config::getNext()");
  ASSERT(cnt > 0);
  ASSERT(key);

  log.add(5,"info: config::getNext(%ld, '%s')", cnt, key);

  currValue = values.getFirst();
  while (currValue)
  {
    if (!strcmp(currValue->getKey(), key))
      if (--cnt == 0)
        return(currValue->getVal());

    currValue = values.getNext(currValue);
  }
  return(NULL);
}

// dumping list to stdout
VOID Config::dump(VOID)
{
  KeyValue * currValue = NULL;
  INT x=0,y1=0,y2,z1=0,z2;

  DEBUGGER("Config::dump()");

  log.add(0,"\nDumping \"%s\":\n\n", name.c_str());

  currValue = values.getFirst();
  while (currValue)
  {
    log.add(0,"%03d: %-25s = \"%s\"", ++x, currValue->getKey(), currValue->getVal());
    
    y2 = strlen(currValue->getKey());
    z2 = strlen(currValue->getVal());
    if (y2 > y1) y1=y2;
    if (z2 > z1) z1=z2;

    currValue = values.getNext(currValue);
  }
  log.add(0,"\nNum: %d, max keylen: %d, max valuelen: %d", x, y1, z1);
}

// dumping list to socket
VOID Config::dump(Connection * conn)
{
  SBYTE buf[MAX_KEY_VAL_LINE_SIZE];
  KeyValue * currValue = NULL;

  DEBUGGER("Config::dump(Connection)");
  ASSERT(conn);

  currValue = values.getFirst();
  while (currValue)
  {
    // do not dump temporary entries
    if (!(currValue->options | CONF_TEMP));
    {
      snprintf(buf, MAX_DATA_SIZE, "250-\"%s\" \"%s\" \"%d\"\r\n",
        currValue->getKey(), currValue->getVal(), currValue->options);
      conn->cwrite(buf);
    }
    currValue = values.getNext(currValue);
  }
}

// dumping keys to socket
VOID Config::dumpKeys(Connection * conn)
{
  SBYTE buf[MAX_KEY_VAL_LINE_SIZE];
  KeyValue * currKey = NULL;
  SBYTE * val = NULL;

  DEBUGGER("Config::dumpKeys(Connection)");
  ASSERT(conn);

  currKey = keys.getFirst();
  while (currKey)
  {
    val = currKey->getVal();
    snprintf(buf, MAX_DATA_SIZE, "250-\"%s\" \"%s\" \"%d\" \"%d\"\r\n",
                currKey->getKey(), val?val:"", currKey->vtype, currKey->options);
    conn->cwrite(buf);
    currKey = keys.getNext(currKey);
  }
}

// creating/saving config
BOOL Config::write(const SBYTE * n)
{
  KeyValue * currValue = NULL, * tmp = NULL;
  Storage * stor = NULL;
  DataBlock cfgblock(TYPE_KEYVAL);
 
  DEBUGGER("Config::write()");

  // writing all config-entries into storage
  if (!strcasecmp(n, "database"))
  {
    stor = new DBStorage(STORE_CONF, MODE_WRITE);
    n = (SBYTE *) name.c_str();
  }
  else
    stor = new FileStorage(STORE_CONF, MODE_WRITE);

  ASSERT(stor);
  if (stor->open(n) == TRUE)
  {
    currValue = values.getFirst();
    while (currValue)
    {
      // do not write temporary keys to disk
      if (!(currValue->options & CONF_TEMP))
      {
        // since values have to stay in config we need to copy the values here by hand
        tmp = new KeyValue(currValue->getKey(), TYPE_STR, CONF_NONE, currValue->getVal());
        cfgblock.add(tmp);
      }
      currValue = values.getNext(currValue);
    }

    if (stor->add(&cfgblock) == FALSE)
    {
      log.add(1,"error: can't write into config '%s', exiting", n);
      exit(1);
    }
    stor->close();
    delete stor;
  }
  else
  {
    log.add(1,"error: can't open config '%s' for writing, exiting", n);
    exit(1);
  }
  return(TRUE);
}

// deleting given key from list
BOOL Config::delValue(const SBYTE * key, SBYTE * val = NULL)
{
  KeyValue * curr = NULL, * next = NULL;
  BOOL found = FALSE;

  log.add(5,"Config::delValue('%s','%s')", key?key:"(null)", val?val:"(null)");

  if (!key)
  {
    log.add(2,"error: config got null pointer as key to delete");
    return(FALSE);
  }

  curr = values.getFirst();
  while (curr)
  {
    next = values.getNext(curr);

    // if the key matches
    if (!strcmp(curr->getKey(), key))
    {
      found = TRUE;

      // and the value is not protected
      if (!(curr->options & CONF_PROT))
      {
        // delete all multiple key/values if no value has been set
        if (!val)
        {
          log.add(6,"info: dropping config entry '%s'", curr->getKey());
          values.del(curr);
          delete curr;
          return(TRUE);
        }
        else
        {
          if (!strcmp(curr->getVal(), val))
          {
            log.add(6,"info: dropping config entry '%s' = '%s'", key, val);
            values.del(curr);
            delete curr;
            return(TRUE);
          }
          else
            found = FALSE;
        }
      }
      else
        log.add(6,"info: entry '%s' is protected, kept", curr->getKey());
    }
    curr = next;
  }

  if (found == FALSE)
    log.add(4,"warning: tried to delete unknown key '%s' from config", key);

  return(found);
}

// reading config
BOOL Config::read(const SBYTE * n, const BOOL tst = FALSE)
{
  Storage  * stor = NULL;
  KeyValue * tmp = NULL;
  DataBlock  block(TYPE_KEYVAL);
  ULONG num = 0;

  DEBUGGER("Config::read()");

  if (!strcasecmp(n, "database"))
  {
    stor = new DBStorage(STORE_CONF, MODE_READ);
    n = (SBYTE *) name.c_str();
  }
  else
    stor = new FileStorage(STORE_CONF, MODE_READ);

  log.add(4,"info: reading config '%s'", n);

  ASSERT(stor);

  // getting all config-entries from storage
  if (stor->open(n) == TRUE)
  {
    if (stor->get(&block) == TRUE)
    {
      while (block.get(&tmp) == TRUE)
      {
        ASSERT(tmp);
        num ++;
        if (addValue(tmp->getKey(), tmp->getVal()) == FALSE)
        {
          log.add(1,"error: config '%s': invalid key '%s', exiting.", n, tmp->getKey());
          exit(1);
        }  
      }
    }
    else
      log.add(3,"error: can't read values from config '%s' (empty ?)", n);

    stor->close();
    delete stor;
  }
  else
  {
    // lock-files are also config, if tst is set to TRUE, don't exit
    if (tst == FALSE)
    {
      log.add(1,"error: can't read config '%s', exiting", n);
      exit(1);
    }
    else
      log.add(4,"info: no lock '%s'", n);
  }

  if (num)
    return(TRUE);
  else
    return(FALSE);
}

// deleting all entries
VOID Config::clear()
{
  KeyValue * val = NULL, * tmp = NULL;

  DEBUGGER("Config::clear()");

  val = values.getFirst();
  while (val)
  {
    tmp = values.getNext(val);

    // only delete non protected entries
    if (!(val->options & CONF_PROT))
    {
      log.add(6,"info: dropping config entry '%s'", val->getKey());
      values.del(val);
      delete val;
    }
    else
      log.add(6,"info: entry '%s' is protected, kept", val->getKey());

    val = tmp;
  }
}

// removing config
BOOL Config::remove(SBYTE * n)
{
  KeyValue * val = NULL, * tmp = NULL;
  Storage  * stor = NULL;

  DEBUGGER("Config::remove()");
  ASSERT(n);

  // delete values in memory
  val = values.getFirst();
  while (val)
  {
    tmp = values.getNext(val);
    values.del(val);
    delete val;
    val = tmp;
  }

  // delete config
  if (!strcasecmp(n, "database"))
  {
    stor = new DBStorage(STORE_CONF, MODE_DELETE);
    n = (SBYTE *) name.c_str();
  }
  else
    stor = new FileStorage(STORE_CONF, MODE_DELETE);

  log.add(4,"info: deleting config '%s'", n);

  ASSERT(stor);
  if (stor->open(n) == TRUE)
  {
    stor->destroy(n);
    stor->close();
  }
  else
    log.add(4,"warning: cannot open source '%s', deletion failed", n);

  delete stor;
  return(TRUE);
}
