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
#include "dbstorage_def.h"
#include "database_def.h"
#include "log_def.h"

#include "line_def.h"
#include "address_inl.h"
#include "keyvalue_inl.h"
#include "datablock_inl.h"

extern Log log;
extern Config config;
extern Timer timer;

SBYTE * NO_DB_ERR = {"error: database support not configured, please recompile"};
SBYTE * CONFIG_TABLE = {"limezconfig"};
SBYTE * SENDER_TABLE = {"limezsender"};

DBStorage::DBStorage(STORECLASS sclass, FILEMODE fmode)
{
#ifdef USE_DATABASE
  this->fmode = fmode;
  this->sclass = sclass;
  opened = FALSE;
  db = NULL;
#endif
}

DBStorage::~DBStorage()
{
#ifdef USE_DATABASE
  ASSERT(!opened);
  ASSERT(!db);
#endif
}

// try to open table, if the given table not exists, create
BOOL DBStorage::open(const SBYTE * table)
{
#ifdef USE_DATABASE
  DEBUGGER("DBStorage::open()");
  ASSERT(table);
  ASSERT(!opened);
  
  strncpy(name, table, MAX_VAL_SIZE);

  // trying to connect a couple of times
  for (SBYTE count = 0; count < 3; count ++)
  {
    db = new Database;
    ASSERT(db);

    // connecting to database
    if (db->connect(config.getValue(KEY_DB_HOST), config.getValue(KEY_DB_NAME),
        config.getValue(KEY_DB_USER), config.getValue(KEY_DB_PASS)) == FALSE)
    {
      delete db;
      db = NULL;
      log.add(2,"error: connect to database failed, trying again");
      sleep(1);
    }
    else
    {
      opened = TRUE;
      break;
    }
  }

  if (!opened)
  {
    log.add(1,"error: connect to database failed, giving up");
    return(FALSE);
  }

  switch(sclass)
  {
    case(STORE_CONF):
    {
      DEBUGGER("DBStorage::open():STORE_CONF");

      // checking if table already exists and create if not
      snprintf((SBYTE *)sqlbuffer, SQL_BUF_SIZE, "select count(*) from %s", CONFIG_TABLE);
    
      if (db->getCount((SBYTE *)sqlbuffer)<0)
      {
        // creating table
        snprintf((SBYTE *)sqlbuffer, SQL_BUF_SIZE,
          "create table %s (entry_key CHAR(%ld) NOT NULL, entry_value CHAR(%ld) NOT NULL, listname CHAR(%ld) NOT NULL)",
          CONFIG_TABLE, (ULONG) MAX_KEY_SIZE, (ULONG) MAX_VAL_SIZE, (ULONG) MAX_SMTP_USER_SIZE);

        if (!db->runNonSelect((SBYTE *)sqlbuffer))
        {
          log.add(1,"error: table '%s' did not exist, creation failed", CONFIG_TABLE);
          this->close();
          return(FALSE);
        }         
        else
          log.add(1,"info: table '%s' did not exist, successfully created", CONFIG_TABLE);

        // creating index
        snprintf((SBYTE *)sqlbuffer, SQL_BUF_SIZE,
          "create index idxconfig on %s (%s)", CONFIG_TABLE, "listname");

        if (!db->runNonSelect((SBYTE *)sqlbuffer))
        {
          log.add(1,"error: index for '%s' did not exist, creation failed", CONFIG_TABLE);
          this->close();
          return(FALSE);
        }
        else
          log.add(1,"info: index for '%s' did not exist, successfully created", CONFIG_TABLE);

      }
      log.add(4,"database: table '%s' opened", CONFIG_TABLE);
      return(TRUE);
      break;
    }

    case(STORE_SENDER):
    {
      DEBUGGER("DBStorage::open():STORE_SENDER");

      // checking if table already exists and create if not
      snprintf((SBYTE *)sqlbuffer, SQL_BUF_SIZE, "select count(*) from %s", SENDER_TABLE);

      if (db->getCount((SBYTE *)sqlbuffer)<0)
      {
        // creating table
        snprintf((SBYTE *)sqlbuffer, SQL_BUF_SIZE,
          "create table %s (uname CHAR(%ld) NOT NULL,udomain CHAR(%ld) NOT NULL, listname CHAR(%ld) NOT NULL)",
          SENDER_TABLE, (ULONG) MAX_KEY_SIZE, (ULONG) MAX_VAL_SIZE, (ULONG) MAX_SMTP_USER_SIZE);

        if (!db->runNonSelect((SBYTE *)sqlbuffer))
        {
          log.add(1,"error: table '%s' did not exist, creation failed", SENDER_TABLE);
          this->close();
          return(FALSE);
        }
        else
          log.add(1,"info: table '%s' did not exist, successfully created", SENDER_TABLE);

        // creating index
        snprintf((SBYTE *)sqlbuffer, SQL_BUF_SIZE,
          "create index idxsender on %s (%s)", SENDER_TABLE, "listname");

        if (!db->runNonSelect((SBYTE *)sqlbuffer))
        {
          log.add(1,"error: index for '%s' did not exist, creation failed", SENDER_TABLE);
          this->close();
          return(FALSE);
        }
        else
          log.add(1,"info: index for '%s' did not exist, successfully created", SENDER_TABLE);
      }
      log.add(4,"database: table '%s' opened", SENDER_TABLE);
      return(TRUE);
      break;
    }

    case(STORE_USER):
    {
      DEBUGGER("DBStorage::open():STORE_USER");

      // checking if table already exists and create if not
      snprintf((SBYTE *)sqlbuffer, SQL_BUF_SIZE, "select count(*) from %s", table);

      // create table
      if (db->getCount((SBYTE *)sqlbuffer)<0)
      {
        snprintf((SBYTE *)sqlbuffer, SQL_BUF_SIZE,
          "create table %s (uname CHAR(%ld) NOT NULL, udomain CHAR(%ld) NOT NULL, fname CHAR(%ld), lname CHAR(%ld), susp CHAR(5), dig CHAR(5) , cdate CHAR(32) NOT NULL, edate CHAR(32))",
          table, (ULONG) MAX_KEY_SIZE, (ULONG) MAX_VAL_SIZE, (ULONG) MAX_FIRSTNAME_SIZE,
          (ULONG) MAX_LASTNAME_SIZE);

        if (!db->runNonSelect((SBYTE *)sqlbuffer))
        {
          log.add(1,"error: table '%s' did not exist, creation failed", table);
          this->close();
          return(FALSE);
        }
        else
          log.add(1,"info: table '%s' did not exist, successfully created", table);

        // create address index
        snprintf((SBYTE *)sqlbuffer, SQL_BUF_SIZE,
          "create unique index idx%s on %s (uname, udomain)", table, table);

        if (!db->runNonSelect((SBYTE *)sqlbuffer))
        {
          log.add(1,"error: index for '%s' did not exist, creation failed", table);
          this->close();
          return(FALSE);
        }
        else
          log.add(1,"info: index for '%s' did not exist, successfully created", table);
      }
      log.add(4,"database: table '%s' opened", table);
      return(TRUE);
      break;
    }

    default:
      return(FALSE);
      break;
  }
  return(FALSE);

#else
  log.add(1, NO_DB_ERR);
  return(FALSE);
#endif
}

// close connection to database
BOOL DBStorage::close(VOID)
{
#ifdef USE_DATABASE
  DEBUGGER("DBStorage::close()");
  ASSERT(opened);
  ASSERT(db);

  db->clearResults();
  db->disconnect();
  delete db;
  db = NULL;
  opened = FALSE;
  return(TRUE);

#else
  log.add(1, NO_DB_ERR);
  return(FALSE);
#endif
}

// add data to db
BOOL DBStorage::add(DataBlock * block)
{                 
#ifdef USE_DATABASE
  DEBUGGER("DBStorage::add()");
  ASSERT(block);
  ASSERT(opened);
  ASSERT(fmode == MODE_WRITE || fmode == MODE_APPEND);

  switch(sclass)
  {
    case(STORE_CONF):
    {
      KeyValue * keyval = NULL;

      DEBUGGER("DBStorage::add():STORE_CONF");
      ASSERT(block->getType()==TYPE_KEYVAL);

      // delete all values first
      if (fmode == MODE_WRITE)
      {
        snprintf((SBYTE *) sqlbuffer, SQL_BUF_SIZE, "delete from %s where listname='%s'", CONFIG_TABLE, name);

        // executing delete statement
        if (!db->runNonSelect((SBYTE *)&sqlbuffer))
        {
          log.add(1,"error: delete of all '%s' values from table '%s' failed", name, CONFIG_TABLE);
          return(FALSE);
        }
        else
          log.add(4,"info: all '%s' values from table '%s' deleted", name, CONFIG_TABLE);
      }

      // insert all values in block
      while (block->get(&keyval) == TRUE)
      {
        snprintf((SBYTE *) sqlbuffer, SQL_BUF_SIZE,
          "insert into %s (entry_key, entry_value, listname) values('%s','%s','%s')",
          CONFIG_TABLE, keyval->getKey(), keyval->getVal(), name);

        // executing statement
        if (!db->runNonSelect((SBYTE *)&sqlbuffer))
        {
          log.add(1,"error: can't store value '%s'='%s' (%s) in table '%s'",
            keyval->getKey(), keyval->getVal(), name, CONFIG_TABLE);
          return(FALSE);
        }
        else
          log.add(4,"info: key '%s'='%s' (%s) stored in table '%s'",
            keyval->getKey(), keyval->getVal(), name, CONFIG_TABLE);
      }
      return(TRUE);
      break;
    }

    case(STORE_SENDER):
    {
      Address * adr = NULL;

      DEBUGGER("DBStorage::add():STORE_SENDER");
      ASSERT(block->getType()==TYPE_ADDRESS);

      // insert all values in block
      while (block->get(&adr) == TRUE)
      {
        snprintf((SBYTE *) sqlbuffer, SQL_BUF_SIZE,
                 "insert into %s (uname,udomain,listname) values('%s','%s','%s')",
                 SENDER_TABLE, adr->getUser(), adr->getDomain(), name);

        // executing statement
        if (!db->runNonSelect((SBYTE *)&sqlbuffer))
        {
          log.add(1,"error: can't store sender '%s@%s' (%s) in table '%s'",
                    adr->getUser(), adr->getDomain(), name, SENDER_TABLE);
          return(FALSE);
        }
        else
          log.add(4,"info: sender '%s@%s' (%s) stored in table '%s'",
                    adr->getUser(), adr->getDomain(), name, SENDER_TABLE);
      }
      return(TRUE);
      break;
    }

    case(STORE_USER):
    {
      Address * adr = NULL;

      DEBUGGER("DBStorage::add():STORE_USER");
      ASSERT(block->getType()==TYPE_ADDRESS);

      // insert all values in block
      while (block->get(&adr) == TRUE)
      {
        //log.add(4,"write: user = '%s@%s'", adr->getUser(), adr->getDomain());
        snprintf((SBYTE *) sqlbuffer, SQL_BUF_SIZE,
                 "insert into %s (uname,udomain,cdate) values('%s','%s','%ld')",
                 name, adr->getUser(), adr->getDomain(), timer.getUnix());

        // executing statement
        if (!db->runNonSelect((SBYTE *)&sqlbuffer))
        {
          log.add(1,"error: can't store users '%s@%s' in table '%s'",
                    adr->getUser(), adr->getDomain(), name);
          return(FALSE);
        }
        else
          log.add(4,"info: user '%s@%s' stored in table '%s'",
                    adr->getUser(), adr->getDomain(), name);
      }
      return(TRUE);
      break;
    }

    default:
      return(FALSE);
      break;
  }
  return(FALSE);

#else
  log.add(1, NO_DB_ERR);
  return(FALSE);
#endif
}

// get data from database
BOOL DBStorage::get(DataBlock * block, ULONG start = 1, ULONG count = 0)
{     
#ifdef USE_DATABASE
  DEBUGGER("DBStorage::get()");
  ASSERT(opened);
  ASSERT(fmode == MODE_READ);
  ASSERT(block);

  BOOL got = FALSE;
  SBYTE ** r = NULL;

  switch(sclass)
  {
    case(STORE_CONF):
    {
      KeyValue * newval = NULL;

      DEBUGGER("DBStorage::get():STORE_CONF");
      ASSERT(block->getType() == TYPE_KEYVAL);

      snprintf((SBYTE *)sqlbuffer, SQL_BUF_SIZE,
               "select entry_key, entry_value from %s where listname='%s'", CONFIG_TABLE, name);

      if (!db->runSelect((SBYTE *)sqlbuffer))
      {
        log.add(1,"error: no values for listname '%s' in table '%s'", name, CONFIG_TABLE);
        return(FALSE);
      }         

      while ((r = db->getRow()))
      {
        ASSERT(r);
        ASSERT((SBYTE *) r[0]);
        ASSERT((SBYTE *) r[1]);
    
        if ((strlen(r[0]) >= MAX_KEY_SIZE) || (strlen(r[0]) < 1))
        {
          log.add(1,"error: config (%s), key '%s': invalid keysize, ignored", name, r[0]);
          continue;
        }
    
        if ((strlen(r[1]) >= MAX_VAL_SIZE) || (strlen(r[1]) < 1))
        {
          log.add(1,"error: config (%s), key '%s': invalid valuesize, ignored", name, r[1]);
          continue;
        }

        newval = new KeyValue(r[0], TYPE_STR, 0, r[1]);
        ASSERT(newval);
        got = TRUE;
        block->add(newval);
      }
      db->clearResults();
      return(got);
      break;
    }

    case(STORE_SENDER):
    {
      Address * adr = NULL;
      SBYTE * ret = NULL;

      DEBUGGER("DBStorage::get():STORE_SENDER");
      ASSERT(block->getType()==TYPE_ADDRESS);

      snprintf((SBYTE *)sqlbuffer, SQL_BUF_SIZE,
        "select uname, udomain from %s where listname='%s'", SENDER_TABLE, name);

      if (!db->runSelect((SBYTE *)sqlbuffer))
      {
        log.add(1,"error: no values for '%s' in table '%s'", name, SENDER_TABLE);
        return(FALSE);
      }

      // get all requested senders from database
      while ((r = db->getRow()))
      {
        adr = new Address;
        ASSERT(adr);
        ASSERT(r);
        ASSERT((SBYTE *) r[0]);
        ASSERT((SBYTE *) r[1]);

        if ((ret = adr->setUserName((SBYTE *) r[0])))
        {
          log.add(2,"error: '%s', user: %s", ret, (SBYTE *) r[0]);
          delete adr;
          continue;
        }

        if ((ret = adr->setDomainName((SBYTE *) r[1])))
        {
          log.add(2,"error: '%s', domain: %s", ret, (SBYTE *) r[1]);
          delete adr;
          continue;
        }

        log.add(5,"info: getting sender '%s@%s' from db", adr->getUser(), adr->getDomain());
        got = TRUE;
        block->add(adr);
      }
      db->clearResults();
      return(got);
      break;
    }

    case(STORE_USER):
    {
      Address * adr = NULL;
      SBYTE * ret = NULL;

      DEBUGGER("DBStorage::get():STORE_USER");
      ASSERT(block->getType()==TYPE_ADDRESS);

#ifdef USE_MYSQL_DB
      snprintf((SBYTE *)sqlbuffer, SQL_BUF_SIZE,
        "select uname,udomain from %s LIMIT %ld, %ld", name, start-1, count);
#endif
#ifdef USE_POSTGRES_DB
      snprintf((SBYTE *)sqlbuffer, SQL_BUF_SIZE,
        "select uname,udomain from %s LIMIT %ld, %ld", name, count, start-1);
#endif

      if (!db->runSelect((SBYTE *)sqlbuffer))
      {
        log.add(1,"error: failed to read values from table '%s'", name);
        return(FALSE);
      }

      // get all requested senders from database
      while ((r = db->getRow()))
      {
        adr = new Address;

        ASSERT(adr);
        ASSERT((SBYTE *) r[0]);
        ASSERT((SBYTE *) r[1]);

        if ((ret = adr->setUserName((SBYTE *) r[0])))
        {
          log.add(2,"error: '%s', user: %s", ret, (SBYTE *) r[0]);
          delete adr;
          continue;
        }

        if ((ret = adr->setDomainName((SBYTE *) r[1])))
        {
          log.add(2,"error: '%s', domain: %s", ret, (SBYTE *) r[1]);
          delete adr;
          continue;
        }

        log.add(5,"info: getting user '%s@%s' from db", adr->getUser(), adr->getDomain());
        got = TRUE;
        block->add(adr);
      }
      db->clearResults();
      return(got);
      break;
    }

    default:
      return(FALSE);
      break;
  }
  return(FALSE);

#else
  log.add(1, NO_DB_ERR);
  return(FALSE);
#endif
}

// delete some entries
BOOL DBStorage::remove(DataBlock * block)
{
#ifdef USE_DATABASE
  DEBUGGER("DBStorage::remove()");
  ASSERT(opened);
  ASSERT(fmode == MODE_DELETE);
  ASSERT(block);

  switch(sclass)
  {
    case(STORE_CONF):
    {
      KeyValue * keyval = NULL;

      DEBUGGER("DBStorage::remove():STORE_CONF");
      ASSERT(block->getType() == TYPE_KEYVAL);

      // remove all values in block
      while (block->get(&keyval) == TRUE)
      {
        snprintf((SBYTE *)sqlbuffer, SQL_BUF_SIZE,
                 "delete from %s where listname='%s' AND entry_key='%s' AND entry_value='%s'",
                 CONFIG_TABLE, name, keyval->getKey(), keyval->getVal());

        if (!db->runNonSelect((SBYTE *)&sqlbuffer))
        {
          log.add(1,"error: failed to remove value '%s'='%s' (%s) from table '%s'",
                    keyval->getKey(), keyval->getVal(), name, CONFIG_TABLE);
          return(FALSE);
        }
        else
          log.add(4,"info: key '%s'='%s' (%s) removed from table '%s'",
                    keyval->getKey(), keyval->getVal(), name, CONFIG_TABLE);
      }
      return(TRUE);
      break;
    }

    case(STORE_USER):
    {
      Address * adr = NULL;

      DEBUGGER("DBStorage::remove():STORE_USER");
      ASSERT(block->getType() == TYPE_ADDRESS);

      // remove all user in block
      while (block->get(&adr) == TRUE)
      {
        snprintf((SBYTE *)sqlbuffer, SQL_BUF_SIZE,
                 "delete from %s WHERE uname='%s' AND udomain='%s'",
                 name, adr->getUser(), adr->getDomain());

        if (!db->runNonSelect((SBYTE *)&sqlbuffer))
        {
          log.add(1,"error: failed to remove user '%s@%s' from table '%s'",
                    adr->getUser(), adr->getDomain(), name);
          return(FALSE);
        }
        else
          log.add(4,"info: user '%s@%s' removed from table '%s'",
                    adr->getUser(), adr->getDomain(), name);
      }

      return(TRUE);
      break;
    }

    case(STORE_SENDER):
    {
      Address * adr = NULL;

      DEBUGGER("DBStorage::remove():STORE_SENDER");
      ASSERT(block->getType() == TYPE_ADDRESS);

      // remove all sender in block
      while (block->get(&adr) == TRUE)
      {
        snprintf((SBYTE *)sqlbuffer, SQL_BUF_SIZE,
                 "delete from %s where listname='%s' AND uname='%s' AND udomain='%s'",
                 SENDER_TABLE, name, adr->getUser(), adr->getDomain());

        if (!db->runNonSelect((SBYTE *)&sqlbuffer))
        {
          log.add(1,"error: failed to remove sender '%s@%s' (%s) from table '%s'",
                    adr->getUser(), adr->getDomain(), name, SENDER_TABLE);
          return(FALSE);
        }
        else
          log.add(4,"info: sender '%s@%s' (%s) removed from table '%s'",
                    adr->getUser(), adr->getDomain(), name, SENDER_TABLE);
      }

      return(TRUE);
      break;
    }

    default:
      return(FALSE);
      break;
  }
  return(FALSE);

#else
  log.add(1, NO_DB_ERR);
  return(FALSE);
#endif
}

// change entries in database
BOOL DBStorage::change(DataBlock * block)
{
#ifdef USE_DATABASE
  DEBUGGER("DBStorage::change()");
  ASSERT(opened);
  ASSERT(fmode == MODE_READ);
  ASSERT(block);

  log.add(1,"DBStorage::change() not implemented yet");
  return(FALSE);

#else
  log.add(1, NO_DB_ERR);
  return(FALSE);
#endif
}

// drop vall values from table
BOOL DBStorage::destroy(SBYTE * nam)
{
#ifdef USE_DATABASE
  DEBUGGER("DBStorage::destroy()");
  ASSERT(opened);

  switch(sclass)
  {
    case(STORE_CONF):
    {
      DEBUGGER("DBStorage::destroy():STORE_CONF");
      snprintf((SBYTE *)sqlbuffer, SQL_BUF_SIZE, "delete from %s where listname='%s'", CONFIG_TABLE, nam);

      if (!db->runNonSelect((SBYTE *)&sqlbuffer))
      {
        log.add(1,"error: failed to remove config for '%s' from table '%s'", nam, CONFIG_TABLE);
        return(FALSE);
      }
      else
        log.add(4,"info: config for '%s' removed from table '%s'", nam, CONFIG_TABLE);

      return(TRUE);
      break;
    }

    case(STORE_SENDER):
    {
      DEBUGGER("DBStorage::destroy():STORE_SENDER");
      snprintf((SBYTE *)sqlbuffer, SQL_BUF_SIZE, "delete from %s where listname='%s'", SENDER_TABLE, nam);

      if (!db->runNonSelect((SBYTE *)&sqlbuffer))
      {
        log.add(1,"error: failed to remove sender for '%s' from table '%s'", nam, SENDER_TABLE);
        return(FALSE);
      }
      else
        log.add(4,"info: sender for '%s' removed from table '%s'", nam, SENDER_TABLE);

      return(TRUE);
      break;
    }

    case(STORE_USER):
    {
      DEBUGGER("DBStorage::destroy():STORE_USER");
      snprintf((SBYTE *)sqlbuffer, SQL_BUF_SIZE, "drop table %s", nam);

      if (!db->runNonSelect((SBYTE *)&sqlbuffer))
      {
        log.add(1,"error: failed to remove table '%s'", nam);
        return(FALSE);
      }
      else
        log.add(4,"info: table '%s' destroyed", nam);

      return(TRUE);
      break;
    }

    default:
      return(FALSE);
      break;
  }
  return(FALSE);

#else
  log.add(1, NO_DB_ERR);
  return(FALSE);
#endif
}

// rename table
BOOL DBStorage::reName(const SBYTE * newname)
{
#ifdef USE_DATABASE
  DEBUGGER("DBStorage::reName()");
  ASSERT(newname);
  ASSERT(name);
  log.add(1,"DBStorage::reName() not implemented yet");
  return(FALSE);

#else
  log.add(1, NO_DB_ERR);
  return(FALSE);
#endif
}

// getNumber
ULONG DBStorage::getCount(VOID)
{
#ifdef USE_DATABASE
  ULONG num = 0;

  DEBUGGER("DBStorage::getCount()");
  ASSERT(opened);
  ASSERT(name);
  ASSERT(fmode == MODE_READ);

  switch(sclass)
  {
    case(STORE_CONF):
    {
      DEBUGGER("DBStorage::getCount():STORE_CONF");
      snprintf((SBYTE *)sqlbuffer, SQL_BUF_SIZE,
               "select count(*) from %s where listname='%s'", CONFIG_TABLE, name);

      if ((num = db->getCount((SBYTE *)sqlbuffer)) < 0)
      {
        log.add(1,"error: getcount of table '%s' failed", CONFIG_TABLE);
        num = 0;
      }
      break;
    }

    case(STORE_SENDER):
    {
      DEBUGGER("DBStorage::getCount():STORE_USER");
      snprintf((SBYTE *)sqlbuffer, SQL_BUF_SIZE,
               "select count(*) from %s where listname='%s'", SENDER_TABLE, name);

      if ((num = db->getCount((SBYTE *)sqlbuffer)) < 0)
      {
        log.add(1,"error: getcount of table '%s' failed", SENDER_TABLE);
        num = 0;
      }
      break;
    }

    case(STORE_USER):
    {
      DEBUGGER("DBStorage::getCount():STORE_USER");
      snprintf((SBYTE *)sqlbuffer, SQL_BUF_SIZE,
               "select count(*) from %s", name);

      if ((num = db->getCount((SBYTE *)sqlbuffer)) < 0)
      {
        log.add(1,"error: getcount of table '%s' failed", name);
        num = 0;
      }
      break;
    }

    default:
      return(FALSE);
      break;
  }
  return(num);

#else
  log.add(1, NO_DB_ERR);
  return(0);
#endif
}

// find entry in database
BOOL DBStorage::find(DataBlock * block)
{
#ifdef USE_DATABASE
  DEBUGGER("DBStorage::find()");
  ASSERT(opened);
  ASSERT(fmode == MODE_READ);
  ASSERT(block);

  BOOL got = FALSE;
  ULONG num = 0;

  switch(sclass)
  {
    case(STORE_CONF):
    {
      KeyValue * tmp = NULL;

      DEBUGGER("DBStorage::find():STORE_CONF");
      ASSERT(block->getType() == TYPE_KEYVAL);

      if (block->get(&tmp) == TRUE)
      {
        ASSERT(tmp);
        snprintf((SBYTE *)sqlbuffer, SQL_BUF_SIZE,
          "select count(*) from %s where listname='%s' and entry_key='%s' and entry_val='%s'",
          CONFIG_TABLE, name, tmp->getKey(), tmp->getVal());

        if ((num = db->getCount((SBYTE *)sqlbuffer)) < 0)
        {
          log.add(2,"error: getcount of table '%s' failed", CONFIG_TABLE);
          num = 0;
        }

        if (num > 0)
          got = TRUE;
      }
      return(got);
      break;
    }

    case(STORE_SENDER):
    {
      Address * tmp = NULL;

      DEBUGGER("DBStorage::find():STORE_SENDER");
      ASSERT(block->getType() == TYPE_ADDRESS);

      if (block->get(&tmp) == TRUE)
      {
        ASSERT(tmp);
        snprintf((SBYTE *)sqlbuffer, SQL_BUF_SIZE,
          "select count(*) from %s where listname='%s' and uname='%s' and udomain='%s'",
          SENDER_TABLE, name, tmp->getUser(), tmp->getDomain());

        if ((num = db->getCount((SBYTE *)sqlbuffer)) < 0)
        {
          log.add(2,"error: getcount of table '%s' failed", SENDER_TABLE);
          num = 0;
        }

        if (num > 0)
          got = TRUE;
      }
      return(got);
      break;
    }

    case(STORE_USER):
    {
      Address * tmp = NULL;

      DEBUGGER("DBStorage::find():STORE_USER");
      ASSERT(block->getType() == TYPE_ADDRESS);

      if (block->get(&tmp) == TRUE)
      {
        ASSERT(tmp);
        snprintf((SBYTE *)sqlbuffer, SQL_BUF_SIZE,
          "select count(*) from %s where uname='%s' and udomain='%s'",
          name, tmp->getUser(), tmp->getDomain());

        if ((num = db->getCount((SBYTE *)sqlbuffer)) < 0)
        {
          log.add(2,"error: getcount of table '%s' failed", name);
          num = 0;
        }

        if (num > 0)
          got = TRUE;
      }
      return(got);
      break;
    }

    default:
      return(FALSE);
      break;
  }
  return(FALSE);

#else
  log.add(1, NO_DB_ERR);
  return(FALSE);
#endif
}

// copy contents of other storage to current
BOOL DBStorage::clone(const SBYTE * newname)
{
#ifdef USE_DATABASE
  DEBUGGER("DBStorage::clone()");
  ASSERT(opened);
  ASSERT(newname);

  switch(sclass)
  {
    case(STORE_USER):
    {
      DEBUGGER("DBStorage::clone():STORE_USER");

      // creating temporary table
      snprintf((SBYTE *)sqlbuffer, SQL_BUF_SIZE,
        "create table %s (uname CHAR(%ld) NOT NULL, udomain CHAR(%ld) NOT NULL, fname CHAR(%ld), lname CHAR(%ld), susp CHAR(5), dig CHAR(5) , cdate CHAR(32) NOT NULL, edate CHAR(32))",
        newname, (ULONG) MAX_KEY_SIZE, (ULONG) MAX_VAL_SIZE, (ULONG) MAX_FIRSTNAME_SIZE, (ULONG) MAX_LASTNAME_SIZE);

      if (!db->runNonSelect((SBYTE *)&sqlbuffer))
      {
        log.add(1,"error: failed to create temp-table '%s'", newname);
        return(FALSE);
      }
      else
        log.add(4,"info: temp-table '%s' created", newname);

      // copying user from user into tmp-table
      snprintf((SBYTE *)sqlbuffer, SQL_BUF_SIZE,
        "insert into %s select * from %s", newname, name);

      if (!db->runNonSelect((SBYTE *)&sqlbuffer))
      {
        log.add(1,"error: failed to copy users from table '%s' to '%s'", name, newname);
        return(FALSE);
      }
      else
        log.add(4,"info: copied all users from table '%s' to '%s'", name, newname);

      return(TRUE);
      break;
    }

    default:
      return(FALSE);
      break;
  }
  return(FALSE);

#else
  log.add(1, NO_DB_ERR);
  return(FALSE);
#endif
}
