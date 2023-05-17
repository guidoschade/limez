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

#include "log_def.h"
#include "timer_def.h"
#include "config_def.h"
#include "database_def.h"

extern Log log;
extern Timer timer;
extern Config config;  

#ifdef USE_DATABASE
#ifdef USE_MYSQL_DB

// -------------------------------------------------------------------------
// ------------------------ M Y S Q L --------- S E C T I O N --------------
// -------------------------------------------------------------------------

Database::Database()
{ 
  result = NULL;
  myfield = NULL;
  mysql = NULL;
  affectedRows = 0;
  connected = FALSE;
  gotresults = FALSE;
  x = 0;
  y = 0;
}

Database::~Database()
{
}

BOOL Database::connect(SBYTE * dbhost, SBYTE * dbname, SBYTE * dbuser, SBYTE * dbpass)
{
  ASSERT(!connected);
  DEBUGGER("Database::connect(mysql)");

  log.add(8,"db: host='%s',name='%s',user='%s',pass='%s'", dbhost, dbname, dbuser, dbpass);

  if (!dbname)
  {
    log.add(1,"error: no database name (db_name) given but needed for connect");
    return(FALSE);
  }

  mysql = new MYSQL;
  ASSERT(mysql);

  log.add(8,"info: mysql initializing");
  mysql_init(mysql);

  log.add(8,"info: mysql connecting");
  if (!mysql_real_connect(mysql, dbhost, dbuser, dbpass, dbname, 0, NULL, 0))
  {
    log.add(1,"error: connect to db failed (%s)", mysql_error(mysql));
    mysql_close(mysql);
    return(FALSE);
  }

  log.add(4,"info: db connect successful");

  connected = TRUE;
  return(TRUE);
}

VOID Database::disconnect(VOID)
{
  DEBUGGER("Database::disconnect(mysql)");

  if (connected)
  {
    ASSERT(mysql);
    ASSERT(!gotresults);
    mysql_close(mysql);

    if (mysql)
    {
      delete mysql;
      mysql = NULL;
    }
    connected = FALSE;

    log.add(4,"info: db disconnected");
  }
  else
    log.add(1,"error: db not connected, disconnect failed !!!");
}

BOOL Database::runNonSelect(SBYTE * sql)
{
  DEBUGGER("Database::runNonSelect(mysql)");

  if (connected)
  {
    ASSERT(!gotresults);
    log.add(5,"info: cmd '%s'", sql);

    if (mysql_query(mysql, sql))
    {
      log.add(3,"error: db query failed (%s)", mysql_error(mysql));
      return(FALSE);
    }

    log.add(4,"info: db command successful");
    affectedRows = mysql->affected_rows;
    return(TRUE);
  }
  else
  {
    log.add(1,"error: db not connected, query failed !!!");
    return(FALSE);
  }
}

BOOL Database::runSelect(SBYTE * sql)
{
  ULONG rows = 0;

  DEBUGGER("Database::runSelect(mysql)");

  if (connected)
  {
    ASSERT(mysql);
    log.add(5,"info: query '%s'", sql);
    if (mysql_query(mysql, sql))
    {
      log.add(3,"error: db select failed (%s)", mysql_error(mysql));
      return(FALSE);
    }

    log.add(4,"info: db select successful");
    result = mysql_store_result(mysql);
    ASSERT(result);

    rows = mysql->affected_rows;
    log.add(4,"info: query selected %ld row(s)", rows);

    if (rows > 0)
      gotresults = TRUE;

    return(TRUE);
  }
  else
  {
    log.add(1,"error: db not connected, select failed !!!");
    return(FALSE);
  }
}

SBYTE ** Database::getRow(VOID)
{
  DEBUGGER("Database::getRow(mysql)");

  if (connected)
  {
    ASSERT(mysql);

    if (!gotresults)
      return(NULL);

    cur = mysql_fetch_row(result);
    return(cur);
  }
  else
  {
    log.add(1,"error: db not connected, getRow failed !!!");
    return(NULL);
  }
}

VOID Database::clearResults()
{
  DEBUGGER("Database::clearResults(mysql)");

  if (connected)
  {
    ASSERT(mysql);

    if ((mysql->affected_rows > 0) && result && gotresults)
    {
      mysql_free_result(result);
      gotresults = FALSE;
      result = NULL;
      log.add(6,"info: db result buffer freed");
    }
  }
  else
    log.add(1,"error: db not connected, cannot clear results !!!");
}

SLONG Database::getCount(SBYTE * sql)
{
  SBYTE ** r = NULL;
  SLONG resl = 0;

  DEBUGGER("Database::getCount(mysql)");
  ASSERT(sql);
  ASSERT(mysql);
  ASSERT(!gotresults);

  if (runSelect(sql) == TRUE)
  { 
    if ((r = getRow()))
    {
      if ((SBYTE *) r[0])
      {
        resl = atol((SBYTE *) r[0]);
        clearResults();
        return((SLONG) resl); 
      }
    }
    else
      return(-1);
  }

  return(-1);
}

#endif

// -------------------------------------------------------------------------
// ------------------ P O S T G R E S --------- S E C T I O N --------------
// -------------------------------------------------------------------------

#ifdef USE_POSTGRES_DB

Database::Database()
{ 
  affectedRows = 0;
  connected = FALSE;
  gotresults = FALSE;
  x = 0;
  y = 0;
}

Database::~Database()
{
}

BOOL Database::connect(SBYTE * dbhost, SBYTE * dbname, SBYTE * dbuser, SBYTE * dbpass)
{
  DEBUGGER("Database::connect(pg)");

  conn = PQsetdbLogin(dbhost, NULL, NULL, NULL, dbname, dbuser, dbpass);
  if (PQstatus(conn) == CONNECTION_BAD)
  {
    log.add(1,"error: db connect failed (%s)", PQerrorMessage(conn));
    PQfinish(conn);
    return(FALSE);
  }

  log.add(4,"info: db connect successful");

  connected = TRUE;
  return(TRUE);
}

VOID Database::disconnect(VOID)
{
  DEBUGGER("Database::disconnect(pg)");

  if (connected)
  {
    connected = FALSE;
    PQfinish(conn);

    log.add(4,"info: db disconnected");
  }
  else
    log.add(1,"error: db not connected, disconnect failed !!!");
}

BOOL Database::runNonSelect(SBYTE * sql)
{
  DEBUGGER("Database::runNonSelect(pg)");

  x = 0;
  if (connected)
  {
    log.add(5,"info: query '%s'", sql);
    res = PQexec(conn, sql);
    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK)
    {
      log.add(3,"error: db command failed");
      return(FALSE);
    }

    log.add(4,"info: db command successful");
    return(TRUE);
  }
  else
  {
    log.add(1,"error: db not connected, query failed !!!");
    return(FALSE);
  }
}

// execute SQL-Statement which returns values
BOOL Database::runSelect(SBYTE * sql)
{
  DEBUGGER("Database::runSelect(pg)");

  x = 0;
  if (connected)
  {
    log.add(5,"info: query '%s'", sql);
    res = PQexec(conn, sql);
    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK)
    {
      log.add(3,"error: db command failed");
      return(FALSE);
    }
    log.add(4,"info: db command successful");
    gotresults = TRUE;
    return(TRUE);
  }
  else
  {
    log.add(1,"error: db not connected, select failed !!!");
    return(FALSE);
  }
}

// get single row of result
SBYTE ** Database::getRow(VOID)
{
  SBYTE * c = NULL;
  SWORD r = 0;

  DEBUGGER("Database::getRow(pg)");

  if (connected)
  {
    if (!((resptr = new (SBYTE *)[PQnfields(res) * sizeof(SBYTE **)])))
    {
      log.add(2,"error: db query failed, mem allocation error!!!");
      return(NULL);
    }

    if ((x+1) > (ULONG) PQntuples(res))
    {
      delete resptr;
      return(NULL);
    }

    for (y = 0; y < (ULONG) PQnfields(res); y++)
    {
      c = PQgetvalue(res, x, y);

      resptr[y] = c;
      r = strlen(c);

      // removing appended spaces here
      while ((r>=0) && (resptr[y][r]==0x20 || resptr[y][r]=='\0'))
        resptr[y][r--] = '\0';
    }

    x++;
    return(resptr);
  }
  else
  {
    log.add(1,"error: db not connected, getRow failed !!!");
    return(NULL);
  }
}

VOID Database::clearResults(VOID)
{
  DEBUGGER("Database::clearResults(pg)");

  if (connected)
  {
    if (res && gotresults)
    {
      PQclear(res);
      gotresults = FALSE;
      res = NULL;
    }
  }
  else
    log.add(1,"error: db not connected, clear failed !!!");
}

SLONG Database::getCount(SBYTE * sql)
{
  SBYTE result = 0;
  SLONG c = 0;
  SBYTE * cs = NULL;

  DEBUGGER("Database::getCount(pg)");
  ASSERT(sql);

  if ((result = runSelect(sql)) == TRUE)
  {
    cs = PQgetvalue(res, 0, 0);
    clearResults();
    ASSERT(cs);
    log.add(6,"info: getCount(pg) returned '%s' rows", cs);
    c = atol(cs);
    return(c);
  }
  else
    return(-1);
}

#endif
#ifdef USE_MSQL_DB

// -------------------------------------------------------------------------
// ------------- H U G H E S - M S Q L --------- S E C T I O N -------------
// -------------------------------------------------------------------------

Database::Database()
{
  affectedRows = 0;
  connected = FALSE;
  gotresults = FALSE;
  msqlsock = res = 0;
  result = NULL;
  curField = NULL;
}

Database::~Database()
{
}

BOOL Database::connect(SBYTE * dbhost, SBYTE * dbname, SBYTE * dbuser, SBYTE * dbpass)
{
  DEBUGGER("Database::connect(msql)");

  log.add(1,"error: msql could not be supported, lacks functionality");
  exit(0);

  if (!dbname)
  {
    log.add(1,"error: no database name (db_name) given but needed for connect");
    return(FALSE);
  }

  // connecting to server
  if ((msqlsock = msqlConnect(dbhost)) < 0)
  {
    log.add(1,"error: db connect failed (%s)", msqlErrMsg);
    return(FALSE);
  }

  log.add(4,"info: db connect successful");

  // selecting database
  if (msqlSelectDB(msqlsock, dbname) < 0)
  {
    log.add(1,"error: select database failed (%s)", msqlErrMsg);
    msqlClose(msqlsock);
    return(FALSE);
  }

  log.add(4,"info: db selected successful");

  connected = TRUE;
  return(TRUE);
}

VOID Database::disconnect(VOID)
{
  DEBUGGER("Database::disconnect(msql)");

  if (connected)
  {
    ASSERT(msqlsock);
    connected = FALSE;
    msqlClose(msqlsock);

    log.add(4,"info: db disconnected");
  }
  else
    log.add(1,"error: db not connected, disconnect failed !!!");
}

BOOL Database::runNonSelect(SBYTE * sql)
{
  DEBUGGER("Database::runNonSelect(msql)");

  if (connected)
  {
    ASSERT(msqlsock);
    log.add(5,"info: query '%s'", sql);

    if ((res = msqlQuery(msqlsock, sql)) < 0)
    {
      log.add(3,"error: db command failed");
      return(FALSE);
    }

    log.add(4,"info: db command successful");
    return(TRUE);
  }
  else
  {
    log.add(1,"error: db not connected, query failed !!!");
    return(FALSE);
  }
}

// execute SQL-Statement which returns values
BOOL Database::runSelect(SBYTE * sql)
{
  DEBUGGER("Database::runSelect(msql)");

  if (connected)
  {
    ASSERT(msqlsock);
    log.add(5,"info: query '%s'", sql);

    if ((res = msqlQuery(msqlsock, sql)) < 0)
    {
      log.add(3,"error: db command failed");
      return(FALSE);
    }

    log.add(4,"info: db command successful");
    log.add(4,"info: query selected %ld row(s)", res);

    if ((result = msqlStoreResult()) && res > 0)
      gotresults = TRUE;

    return(TRUE);
  }
  else
  {
    log.add(1,"error: db not connected, select failed !!!");
    return(FALSE);
  }
}

SBYTE ** Database::getRow(VOID)
{
  DEBUGGER("Database::getRow(msql)");

  if (connected)
  {
    ASSERT(msqlsock);

    if (!gotresults)
      return(NULL);

    cur = msqlFetchRow(result);
    return(cur);
  }
  else
  {
    log.add(1,"error: db not connected, getRow failed !!!");
    return(NULL);
  }
}

VOID Database::clearResults()
{
  DEBUGGER("Database::clearResults(msql)");

  if (connected)
  {
    ASSERT(msqlsock);

    log.add(6,"info: db freeing result buffer");

    if (result && gotresults)
    {
      msqlFreeResult(result);
      gotresults = FALSE;
      result = NULL;
      log.add(6,"info: db result buffer freed");
    }
  }
  else
    log.add(1,"error: db not connected, cannot clear results !!!");
}

SLONG Database::getCount(SBYTE * sql)
{
  SBYTE ** r = NULL;
  SLONG resl = 0;

  DEBUGGER("Database::getCount(msql)");
  ASSERT(sql);
  ASSERT(msqlsock);
  ASSERT(!gotresults);

  if (runSelect(sql) == TRUE)
  {
    if ((r = getRow()))
    {
      if ((SBYTE *) r[0])
      {
        resl = atol((SBYTE *) r[0]);
        clearResults();
        return((SLONG) resl);
      }
    }
    else
      return(-1);
  }

  return(-1);
}

#endif
#endif
