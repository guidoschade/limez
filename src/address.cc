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

#include "address_def.h"

#include "log_def.h"
#include "list_def.h"
#include "timer_def.h"
#include "resolver_def.h"
#include "listelement_def.h"
#include "mailinglist_def.h"

#include "listelement_inl.h"
#include "list_inl.h"
#include "address_inl.h"

extern Log log;
extern Timer timer;
extern Config config;
extern Resolver dns;
extern List<MailingList> lists;
extern MailingList * getList(const SBYTE * listname, const SBYTE * listdomain = NULL);

Address::Address()
{
  // take care about our status
  mip[0] = mip[1] = mip[2] = 0;
}

Address::~Address()
{
}

// check User name
SBYTE * Address::setUserName(SBYTE * uname)
{
  SBYTE * end = NULL, * tmp = NULL;
  INT len;

  if (!uname)
    return("invalid user size");

  log.add(8,"Address::checkUserName('%s')", uname);

  len = strlen(uname);
  end = uname + len;

  // checking size of useraddr
  if (len > MAX_SMTP_USER_SIZE || len < 1)
    return("invalid user/list name");

  // checking characters in username
  for (tmp = uname; tmp < end; tmp ++)
  {
    // end reached ?
    if (*tmp == 0x20 || *tmp == 0x09 ||
        *tmp == 0x00 || *tmp == 0x0d || *tmp == 0x0a)
    {
      *tmp = '\0';
      break;
    }

    // check for invalid characters
    if (!(((*tmp > 0x40) && (*tmp < 0x5b)) ||
          ((*tmp > 0x60) && (*tmp < 0x7b)) ||
          ((*tmp > 0x2f) && (*tmp < 0x3a)) ||
          ((*tmp > 0x2b) && (*tmp < 0x2f)) ||
          (*tmp == '_')))
      return("invalid user/list name");

    // tolower conversion
    if ((*tmp > 0x40) && (*tmp < 0x5B))
      *tmp += 0x20;
  }

  setUser(uname);
  return(NULL);
}

// check Domain name
SBYTE * Address::setDomainName(SBYTE * udomain)
{
  SBYTE * end = NULL, * tmp = NULL, * dot = NULL;
  INT len;

  if (!udomain)
    return("invalid domain size");

  log.add(8,"Address::checkDomainName('%s')", udomain);

  len = strlen(udomain);
  end = udomain + len;

  // checking size of useraddr
  if (len > MAX_SMTP_DOMAIN_SIZE || len < 1)
    return("wrong domain size");

  // checking characters and size of domain
  for (tmp = udomain; tmp < end; tmp ++)
  {
    // end reached ?
    if (*tmp == 0x20 || *tmp == 0x09 ||
        *tmp == 0x00 || *tmp == 0x0d || *tmp == 0x0a)
    {
      *tmp = '\0';
      break;
    }

    // check for invalid characters
    if (!(((*tmp > 0x40) && (*tmp < 0x5b)) ||
          ((*tmp > 0x60) && (*tmp < 0x7b)) ||
          ((*tmp > 0x2f) && (*tmp < 0x3a)) ||
          ((*tmp > 0x2c) && (*tmp < 0x2f))))
      return("invalid domain name");

    // tolower conversion
    if ((*tmp > 0x40) && (*tmp < 0x5B))
      *tmp += 0x20;
  }

  // checking for dot in domain
  dot = strchr(udomain, '.');
  if (!dot)
    return("invalid domain name");

  setDomain(udomain);
  return(NULL);
}

// set user/domain of address if check did'nt fail
SBYTE * Address::setIfValid(SBYTE * mail, BOOL resolve = TRUE)
{
  SBYTE * dom = NULL;
  SBYTE * res = NULL;
  SBYTE * usr = mail;
  UWORD  size = 0;

  DEBUGGER("Address::checkSet()");

  if (!mail)
    return("wrong address size (null)");

  size = strlen(mail);

  // checking size of mailaddress
  if (size > MAX_SMTP_ADDR_SIZE || size < MIN_SMTP_ADDR_SIZE)
    return("wrong address size");

  // strip leading whitespaces if any
  while(*usr == 0x20 || *usr == 0x09)
    usr++;

  // if we use braces, cut first and last char
  if (*usr == '<')
  {
    if (usr[size-1] != '>')
      return ("braces unbalanced");

    usr[size-1] = '\0';
    usr++;
  }

  // no (at) found
  dom = strchr(usr, '@');
  if (!dom)
    return("wrong domain size");

  // separate user and domain
  *dom = '\0';

  // check + set username
  if ((res = setUserName(usr)) != NULL)
    return(res);

  // restore mail string
  *dom='@';

  // check +set domainname
  if ((res = setDomainName(dom+1)) != NULL)
    return(res);

  // resolve domain if requested
  if (resolve == TRUE)
    if (dns.getExchanger(getDomain(), &mip[0], &mip[1], &mip[2]) == FALSE)
      return("resolve of domain failed");

  return(NULL);
}

// check if recipient is one of our lists or the server itself
RECTYPE Address::isValidRec(BOOL checkdomain = TRUE)
{
  SBYTE * alias = NULL;
  UWORD acount = 0;
 
  DEBUGGER("Address::isValidRec()");

  if (username.size() < 1)
    return (REC_NOTSET);

  if ((checkdomain == TRUE) && (domain.size() < 1))
    return (REC_NOTSET);

  // check server name
  if (username == config.getValue(KEY_SERVER_MAIL))
  {
    if (checkdomain == FALSE)
    {
      log.add(5,"info: server address name '%s' matches", username.c_str());
      return (REC_PROG);
    }
    else
    {
      if (domain == config.getValue(KEY_SERVER_DOMAIN))
      {
        log.add(5,"info: server address '%s@%s' matches", username.c_str(), domain.c_str());
        return(REC_PROG);
      }
    }
  }

  // checking all server aliases
  while((alias = config.getNext(++acount, KEY_SERVER_ALIAS)))
  {
    ASSERT(alias);
    if (username == alias)
    {
      if (checkdomain == FALSE)
      {
        log.add(5,"info: server alias name '%s' matches", username.c_str());
        return(REC_PROG);
      }
      else
      {
        if (domain == config.getValue(KEY_SERVER_DOMAIN))
        {
          log.add(5,"info: server alias '%s@%s' matches", username.c_str(), domain.c_str());
          return(REC_PROG);
        }
      }
    }
  }

  // checking all mailinglists
  if (getList(username.c_str(), (checkdomain == TRUE)?domain.c_str():NULL))
    return(REC_LIST);

  if (checkdomain == TRUE)
    log.add(4,"info: user '%s@%s' is not a list or alias", username.c_str(), domain.c_str());
  else
    log.add(4,"info: username '%s' is not a list or alias", username.c_str());

  return(REC_NONE); 
}

// return FALSE if addresses differ
BOOL Address::compare(Address * adr)
{
  BOOL same = FALSE;

  DEBUGGER("Address::compare()");
  ASSERT(adr);

  if (username == adr->getUser() && domain == adr->getDomain())
    same = TRUE;

  log.add(8,"info: user '%s@%s' %s '%s@%s'", adr->getUser(), adr->getDomain(),
            (same == TRUE)?"matches":"does not match", getUser(), getDomain());
  return(same);
}
