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
#include "lightdir_def.h"

extern Log log;
extern Timer timer;
extern Config config;

#ifdef USE_LDAP

LightDir::LightDir()
{
  result = entry = NULL;
  connected = FALSE;
  gotresults = FALSE;
  ldapsock = NULL;
  maxValues = MAX_LDAP_ENTRIES;
  resptr = NULL;
}

LightDir::~LightDir()
{
}

// connect to the ldap directory
BOOL LightDir::connect(SBYTE * ldaphost, SBYTE * ldapuser, SBYTE * ldappass)
{
  SBYTE res;
  INT max_time = MAX_LDAP_WAIT_TIME;

  DEBUGGER("LightDir::connect(openldap)");
  ASSERT(!connected);

  if (!ldaphost)
  {
    log.add(1,"error: no ldap-host specified, connect failed");
    return(FALSE);
  }

  log.add(6,"ldap: host='%s',user='%s',pass='%s'", ldaphost, ldapuser, ldappass);

  // get handle for LDAP connection
  if (!(ldapsock = ldap_init(ldaphost, DEFAULT_LDAP_PORT)))
  {
    log.add(1,"error: call of ldap_init failed (%s),(%ld)", ldaphost, DEFAULT_LDAP_PORT);
    return(FALSE);
  }

  // setting option (auto reconnect) on
  if ((res = ldap_set_option(ldapsock, LDAP_OPT_RESTART, LDAP_OPT_ON)))
  {
    log.add(1,"error: ldap_set_option failed (%s)", ldap_err2string(res));
    return(FALSE);
  }

  // setting option (follow referrals) off
  if ((res = ldap_set_option(ldapsock, LDAP_OPT_REFERRALS, LDAP_OPT_OFF)))
  {
    log.add(1,"error: ldap_set_option failed (%s)", ldap_err2string(res));
    return(FALSE);
  }

  // setting option (timeout) to 30 seconds
  if ((res = ldap_set_option(ldapsock, LDAP_OPT_TIMELIMIT, (VOID *) &max_time)))
  {
    log.add(1,"error: ldap_set_option failed (%s)", ldap_err2string(res));
    return(FALSE);
  }

  // setting option (size limit) to MAX_LDAP_ENTRIES entries
  if ((res = ldap_set_option(ldapsock, LDAP_OPT_SIZELIMIT, (VOID *) &maxValues)))
  {
    log.add(1,"error: ldap_set_option failed (%s)", ldap_err2string(res));
    return(FALSE);
  }

  // connect and authenticate to the directory as nobody/user
  if ((res = ldap_simple_bind_s(ldapsock, NULL, NULL)))
  {
    log.add(1,"error: bind to ldap-server '%s' failed (%s)", ldaphost, ldap_err2string(res));
    ldap_unbind(ldapsock);
    return(FALSE);
  }

  connected = TRUE;
  return(TRUE);
}

// close connection
VOID LightDir::disconnect(VOID)
{
  DEBUGGER("LightDir::disconnect(openldap)");

  if (connected)
  {
    ASSERT(ldapsock);
    ASSERT(!gotresults);

    ldap_unbind(ldapsock);
    ldapsock = NULL;
    connected = FALSE;

    log.add(4,"info: ldap disconnected");
  }
  else
    log.add(1,"error: ldap not connected, disconnect failed !!!");
}

// search
BOOL LightDir::search(SBYTE * base, BOOL deep = TRUE, SBYTE * filter = NULL, SBYTE ** attrs = NULL)
{
  UWORD entries = 0;
  DEBUGGER("LightDir::search()");

  if(!base)
  {
    log.add(1,"error: no ldap base dn specified");
    return(FALSE);
  }

  // submit query
  if (ldap_search_ext_s(ldapsock, base, deep?LDAP_SCOPE_SUBTREE:LDAP_SCOPE_BASE,
                        filter, attrs, 0, NULL, NULL, NULL, maxValues, &result))
  {
    log.add(2,"error: ldap search failed, base: %s, filter: %s", base, filter);
    return(FALSE);
  }

  if (result)
    entries = ldap_count_entries(ldapsock, result);

  if (!result || !entries)
  {
    log.add(4,"info: query 'base: %s, filter: %s' returned no result/entries", base, filter);
    return(FALSE);
  }
  else
  {
    log.add(5,"info: query 'base: %s, filter: %s' returned %ld entries", base, filter, entries);
    gotresults = TRUE;
  }

  return(TRUE);
}

// clear result buffer
VOID LightDir::clearResults()
{
  DEBUGGER("LightDir::clearResults(openldap)");

  if (connected)
  {
    ASSERT(ldapsock);
    log.add(6,"info: ldap freeing result buffer");

    if (result && gotresults)
    {
      ldap_msgfree(result);
      gotresults = FALSE;
      result = entry = NULL;
      log.add(6,"info: ldap result buffer freed");
    }
  }
  else
    log.add(1,"error: ldap not connected, cannot clear results !!!");
}

// retrieve values from ldap
KeyValue ** LightDir::getEntry(VOID)
{
  SBYTE ** cur = NULL;
  SBYTE * attr = NULL, * dn = NULL;
  INT i, num = 0;
  ULONG numAttributes = 0;

  DEBUGGER("LightDir::getEntry()");

  if (connected)
  {
    ASSERT(ldapsock);

    if (!gotresults)
      return(NULL);

    if (entry)
    {
      // free last result
      entry = ldap_next_entry(ldapsock, entry);
      ASSERT(resptr);
      delete resptr;
      resptr = NULL;
    }
    else
      entry = ldap_first_entry(ldapsock, result);

    // free result and return
    if (!entry)
    {
      if (ber)
        ber_free(ber, 0);

      log.add(5,"info: ldap query fetched last result, buffer freed");
      return(NULL);
    }

    // count number of attributes
    for (attr = ldap_first_attribute(ldapsock, entry, &ber);
         attr; attr = ldap_next_attribute(ldapsock, entry, ber))
    {
      if ((cur = ldap_get_values(ldapsock, entry, attr)))
      {
        numAttributes += ldap_count_values(cur);
        ldap_value_free(cur);
      }
      ldap_memfree(attr);
    }

    // check number
    if (!numAttributes)
    {
      log.add(4,"error: ldap entry has no attributes");
      return(NULL);
    }
    else
      log.add(5,"info: ldap entry contains %ld attributes", numAttributes);

    // allocate memory for structure
    if (!((resptr = new (KeyValue *)[(numAttributes+2) * sizeof(KeyValue **)])))
    {
      log.add(1,"error: ldap query failed, mem allocation error!!!");
      return(NULL);
    }

    // first attribute is distinguished name of entry
    if ((dn = ldap_get_dn(ldapsock, entry)))
    {
      resptr[num++] = new KeyValue("dn", TYPE_STR, CONF_MULT, dn);
      ldap_memfree(dn);
    }

    // get all attributes of entry
    for (attr = ldap_first_attribute(ldapsock, entry, &ber);
         attr; attr = ldap_next_attribute(ldapsock, entry, ber))
    {
      if ((cur = ldap_get_values(ldapsock, entry, attr)) != NULL)
      {
        for (i = 0; cur[i]; i++)
          resptr[num++] = new KeyValue(attr, TYPE_STR, CONF_MULT, cur[i]);

        ldap_value_free(cur);
      }
      ldap_memfree(attr);
    }
    resptr[num] = NULL;
    return(resptr);
  }
  else
  {
    log.add(1,"error: ldap not connected, getEntry failed !!!");
    return(NULL);
  }
}

#endif