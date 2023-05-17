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

#include "log_def.h"
#include "log_inl.h"
#include "config_def.h"
#include "mailinglist_def.h"
#include "lightdir_def.h"

extern Log log;
extern Config config;
extern List<MailingList> lists;

// reading command line arguments
VOID getOptions(INT argc, SBYTE ** argv)
{
  UBYTE par;
  SBYTE * key = NULL, * value = NULL;
  BOOL valid;

#ifdef USE_DATABASE
  SBYTE pwd[MAX_VAL_SIZE+1];
  BOOL pass = FALSE;
#endif

  DEBUGGER("getOptions()");

  // preparing config
  config.clear();
  config.initMainKeys();
  config.setDefaults();

  // parsing command line parameters/options
  for (par=1; par<argc; par++)
  {
    valid = FALSE;

    if (!strcasecmp(argv[par],"--test") || !strcasecmp(argv[par],"-t"))
    {
      config.addValue(KEY_DEBUG_TEST_ONLY, "TRUE", CONF_TEMP);
      valid = TRUE;
    }

    if (!strcasecmp(argv[par],"--version") || !strcasecmp(argv[par],"-v"))
    {
      log.add(0,"Server version :  %s (Unix)", LIMEZSTRING);
      log.add(0,"Server built   :  %s %s", __DATE__, __TIME__);
      log.add(0,"Server options :  + file storage support");

#ifdef USE_LDAP
      log.add(0,"                  + LDAP support (%s %ld)", LDAP_VENDOR_NAME, LDAP_VENDOR_VERSION);
#else
      log.add(0,"                  - NO ldap storage supported");
#endif

#ifdef USE_DATABASE
  #ifdef USE_MYSQL_DB
      log.add(0,"                  + MySQL database support (%s)", MYSQL_SERVER_VERSION);
  #endif
  #ifdef USE_POSTGRES_DB
      log.add(0,"                  + PostgreSQL database support");
  #endif
  #ifdef USE_MSQL_DB
      log.add(0,"                  + MiniSQL database support (not working)");
  #endif
#else
      log.add(0,"                  - NO database storage supported");
#endif

      log.add(0,"");
      log.add(0,"To change the options you need to reconfigure, please refer");
      log.add(0,"to the Limez INSTALL file located in the Limez root.");

      valid = TRUE;
      exit(0);
    }

    // setting config
    if (!strcasecmp(argv[par],"--set") || !strcmp(argv[par],"-s"))
    {
      SBYTE * sw = argv[par];

      if (argc>(par+1))
      {
        key = argv[++par];
        if (argc>(par+1))
        {
          value = argv[++par];
          if (config.addValue(key, value, CONF_PROT) == FALSE)
          {
            log.add(1,"error: configuration problem, exiting.");
            exit(1);
          }
          valid = TRUE;
        }
        else
        {
          log.add(1,"error: switch '%s' requires additional arguments, exiting.", sw);
          exit(1);
        }
      }
      else
      {
        log.add(1,"error: switch '%s' requires two additional arguments, exiting.", sw);
        exit(1);
      }
    }

    if (!strcasecmp(argv[par],"--debug") || !strcmp(argv[par],"-d"))
    {
      config.addValue(KEY_DEBUG_LEVEL, "9", CONF_PROTTEMP);
      log.setDebug(DEBUG_MAX);
      valid = TRUE;
    }

    if (!strcasecmp(argv[par],"--config") || !strcasecmp(argv[par],"-c") || !strcasecmp(argv[par],"-f"))
    {
      if (argc>(par+1))
      {
        config.addValue(KEY_SERVER_CONFIG, argv[++par], CONF_PROTTEMP);
        log.setDebug(9);
        valid = TRUE;
      }
      else
      {
        log.add(1,"error: switch '%s' requires additional argument, exiting.", argv[par]);
        exit(1);
      }
    }

    if (!strcasecmp(argv[par],"--try"))
    {
      config.addValue(KEY_DEBUG_CONNECT_ONLY,"TRUE", CONF_PROTTEMP);
      log.add(1,"warning: test connect mode activated, broadcast mails will not be sent");
      valid = TRUE;
    }

    if (!strcasecmp(argv[par],"--daemon") || !strcmp(argv[par],"-D"))
    {
      config.addValue(KEY_SERVER_DAEMONIZE,"TRUE", CONF_PROTTEMP);
      valid = TRUE;
    }

    if (!strcasecmp(argv[par],"--spool") || !strcmp(argv[par],"-S") || !strcasecmp(argv[par],"-q"))
    {
      config.addValue(KEY_SERVER_SPOOL_ONLY, "TRUE", CONF_PROTTEMP);
      valid = TRUE;
    }

#ifdef USE_DATABASE
    if (!strcasecmp(argv[par],"--pass") || !strcasecmp(argv[par],"-p"))
    {
      valid = TRUE;
      pass = TRUE;
    }
#endif

    if (!strcasecmp(argv[par],"--help") || !strcasecmp(argv[par],"-?") ||
        !strcasecmp(argv[par],"-h") || !valid)
    {
      log.add(0,"usage: %s <options>", argv[0]);
      log.add(0,"valid options are:");
      log.add(0,"--help/-h/-?       - display this help message");
      log.add(0,"--version/-v       - only print version number and compiled options");
      log.add(0,"--test/-t          - check configuration files/tables and exit");
      log.add(0,"--config/-f/-c     - specify alternative configfile (default is 'limez.config')");
      log.add(0,"--try              - do not really send mails, just connect to remote MTA and");
      log.add(0,"                     issue the commands (HELO/MAIL/RCPT/RSET/QUIT)");
      log.add(0,"--debug/-d         - start with maximum debuglevel (only temporarily)");
      log.add(0,"--daemon/-D        - become a daemon (fork process and continue service in bg)");
      log.add(0,"--spool/-S/-q      - spool only, do not listen on given interface,");
      log.add(0,"                     exiting after spoolrun");

#ifdef USE_DATABASE
      log.add(0,"--pass/-p          - database password will be asked interactive, does not");
      log.add(0,"                     appear in processlist (ps -ef) for security reasons");
#endif

      log.add(0,"--set/-s key val   - set <key> of limez mainconfig to given <value>");

#ifdef USE_DATABASE
      log.add(0,"                     (overrides values set in configfile/database, will be saved)");
      log.add(0,"                     if using database-config please specify '-s server_config database'");
      log.add(0,"                     as well as db_host, db_name, db_user, and password via -p");
#endif

      log.add(0,"\nFor more informations about limez please visit \"%s\".\n\n", LIMEZURL);

      exit(0);
    }
  }

#ifdef USE_DATABASE
  // ask for database password interactively
  if (pass)
  {
    printf("password: ");
    if (!fgets(pwd, MAX_VAL_SIZE, stdin))
    {
      log.add(0,"error: failed to read password via stdin, exiting");
      exit(1);
    }

    // crop CR
    pwd[strlen(pwd)-1] = '\0';

    if (config.addValue(KEY_DB_PASS, pwd, CONF_PROT) == FALSE)
    {
      log.add(1,"error: configuration problem, please check logfile, exiting");
      exit(1);
    }
  }
#endif
}

// return pointer to list we were searching for
MailingList * getList(const SBYTE * listname, const SBYTE * listdomain = NULL)
{
  MailingList * ml = NULL;

  DEBUGGER("getList()");
  ASSERT(listname);

  for (ml = lists.getFirst(); ml; ml = lists.getNext(ml))
  {
    if (!strcasecmp(ml->mlconfig.getValue(KEY_LIST_NAME), listname))
    {
      if (listdomain)
      {
        if (!strcasecmp(ml->mlconfig.getValue(KEY_LIST_DOMAIN), listdomain))
        {
          log.add(5,"info: list name/domain '%s@%s' matches", listname, listdomain);
          break;
        }
      }
      else
      {
        log.add(5,"info: list name '%s' matches", listname);
        break;
      }
    }
  }
  return(ml);
}

// get next id for e-mail (message id)
SBYTE * getNextId(VOID)
{
  static ULONG id = 0;
  static SBYTE ids[MAX_INT_STR_SIZE+1];

  DEBUGGER("getNextId()");

  snprintf(ids, MAX_INT_STR_SIZE, "%ld", ++id);
  return((SBYTE *) ids);
}
