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

#ifndef DATABASEDEF_H 
#define DATABASEDEF_H 

#include <stdlib.h>
#include "global.h"

#ifdef USE_MYSQL_DB
  #include "mysql.h"
#endif

#ifdef USE_MSQL_DB
  //  msql v2.x/3.x does not work
  //  because of missing functionality
  //  (no count(*) and no LIMIT x, y)
  #include "msql.h"
#endif

#ifdef USE_POSTGRES_DB
  #include "libpq-fe.h"
#endif

class Database
{ 
  public:

    Database();
    ~Database();

    VOID disconnect(VOID);
    VOID clearResults(VOID);
    BOOL connect(SBYTE * dbhost, SBYTE * dbname, SBYTE * dbuser, SBYTE * dbpass);
    BOOL runNonSelect(SBYTE * sql);
    BOOL runSelect(SBYTE * sql);

    SLONG getCount(SBYTE * sql);
    SBYTE ** getRow(VOID);

  private:
      
    ULONG affectedRows;
    BOOL  connected;
    BOOL  gotresults;

#ifdef USE_MYSQL_DB
    MYSQL_FIELD * myfield;
    MYSQL       * mysql;
    MYSQL_RES   * result;
    MYSQL_ROW     cur;
    INT           x,y;
#endif

#ifdef USE_POSTGRES_DB
    SBYTE       * pghost, * pgport, * pgoptions, * pgtty, * dbName;
    PGconn      * conn;
    PGresult    * res;
    ULONG         x,y;
    SBYTE      ** resptr;
#endif

#ifdef USE_MSQL_DB
    INT           msqlsock, res;
    m_result    * result;
    m_row         cur;
    m_field     * curField;
#endif
};

#endif // #ifndef DATABASEDEF_H 
