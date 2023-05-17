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

#include "resolver_def.h"

extern Log    log;
extern Config config;
extern Timer  timer;

Resolver::Resolver()
{
}

Resolver::~Resolver()
{
}

// reverse mapping
SBYTE * Resolver::reverse(ULONG ip)
{
  static SBYTE * var = "[resolve failed]";
  struct hostent * rhost = NULL;

  DEBUGGER("Resolver::reverse()");

  if (!(rhost = gethostbyaddr((const SBYTE *)&ip, sizeof(struct in_addr), AF_INET)))
  {
    log.add(4,"error: nslookup of '0x%lx' failed (reverse)", ntohl(ip));
    return(var);
  }

  ASSERT(rhost);
  ASSERT(rhost->h_name);
  return(rhost->h_name);
}

// forward mapping
ULONG Resolver::lookup(SBYTE * host)
{
  struct hostent * rhost = NULL;
  ULONG * p = NULL;

  log.add(7,"Resolver::lookup('%s')", host);

  rhost = gethostbyname(host);
  if ((!rhost) || (!rhost->h_addr_list[0]) || (rhost->h_addrtype != AF_INET))
  {
    log.add(3,"error: nslookup of '%s' failed (forward)", host);
    return(0);
  }
  p = (ULONG *) rhost->h_addr_list[0];

  return(ntohl((ULONG)*p));
}

// convert ip to string (dot notation)
SBYTE * Resolver::iptos(ULONG ip)
{
  UBYTE * p = NULL;
  static SBYTE x[IPV4_DEC_DOT_SIZE+1];

  DEBUGGER("Resolver::iptos()");

  p = (UBYTE *) &ip;
  ASSERT(p);

#if BYTE_ORDER == BIG_ENDIAN
  snprintf(x, IPV4_DEC_DOT_SIZE, "%d.%d.%d.%d",(UBYTE)*p,(UBYTE)*(p+1),(UBYTE)*(p+2),(UBYTE)*(p+3));
#else
  snprintf(x, IPV4_DEC_DOT_SIZE, "%d.%d.%d.%d",(UBYTE)*(p+3),(UBYTE)*(p+2),(UBYTE)*(p+1),(UBYTE)*p);
#endif

  log.add(9,"Resolver::iptos() returning '%s'", x);
  return(x);
}

// return three best mail exchanger of given domain
BOOL Resolver::getExchanger(SBYTE * domain, ULONG * ip1, ULONG * ip2, ULONG * ip3)
{
  FILE   * fcache = NULL;
  UBYTE  * mxend = NULL;
  UBYTE  * mxcurr = (UBYTE *) &mxbuffer + HFIXEDSZ;
  HEADER * headstart = (HEADER *) &mxbuffer;

  INT      x, y, z, type, buflen = MAX_SMTP_DOMAIN_SIZE;
  UBYTE    mxrec[MAX_SMTP_DOMAIN_SIZE+1];
  SLONG    numbytes;

  ULONG    currip, num, expire, pos = 0;
  UWORD    pref, mxs[4]={65535,65535,65535,65535};
  ULONG    mip[4]={0,0,0,0};
  
  BOOL      found = FALSE;
  cache_entry ce;

  ASSERT(domain);
  ASSERT(mxcurr);
  ASSERT(headstart);
  ASSERT(ip1);
  ASSERT(ip2);
  ASSERT(ip3);

  log.add(7,"Resolver::getExchanger('%s')", domain);

  // log.add(1,"test: domain='%s', ip1=%ld, ip2=%ld, ip3=%ld", domain, *ip1, *ip2, *ip3);

  // trying to fetch ip's from cache
  if ((fcache = fopen(FILE_DNS_CACHE, "rb")) !=NULL)
  {
    while (TRUE)
    {
      if ((num = fread(&ce, 1, sizeof(cache_entry), fcache)) < 1)
        break;
 
      // log.add(5,"info: read %s, %lx, %lx, %lx, %ld",
      //           ce.name, ce.ip1, ce.ip2, ce.ip3, ce.cdat);

      // check if domain in cache-file matches
      if (!strncasecmp(domain, ce.name, strlen(domain)))
      {
        found = TRUE;
        *ip1 = ce.ip1;
        *ip2 = ce.ip2;
        *ip3 = ce.ip3;

        expire = ce.cdat + config.getIntValue(KEY_SERVER_CACHETIME);

        // log.add(5, "info: dns expire = %ld, time = %ld", expire, timer.getUnix());
        
        // no ip found or dns cache expired
        if (!(*ip1) || timer.getUnix() > expire)
        {
          log.add(4, "info: found domain '%s' in dns cache, expired, getting new", domain);
          pos = ftell(fcache) - sizeof(cache_entry);
          // log.add(4, "info: pos = %ld", pos);
          break;
        }
        else
        {
          log.add(4,"info: found domain '%s' in dns cache", domain);
          fclose(fcache);
          return(TRUE);
        }
      }
    }
    fclose(fcache);
  }
  else
    log.add(2,"error: cache-file '%s' does not exist, creating", FILE_DNS_CACHE);

  mip[0]=0;
  mip[1]=0;
  mip[2]=0;
  mip[3]=0;

  // initialize nameserver
  if (res_init())
  {
    log.add(3,"error: system call res_init() failed, cannot resolve domain '%s'", domain);
    return(FALSE);
  }

  // sending query to nameserver
  if ((numbytes = res_search(domain, C_IN, T_MX, (UBYTE *)& mxbuffer, MAX_PACKET_SIZE)) < 1)
  {
    log.add(3,"error: system call res_search() failed, domain: (%s)", domain);
    return(FALSE);
  }
  
  // make sure everything's okay
  if (numbytes > MAX_PACKET_SIZE)
    numbytes = MAX_PACKET_SIZE;
 
  mxend = (UBYTE *) &mxbuffer + numbytes;
 
  // skip to begin of answer, we still remember our query
  for (x=ntohs(headstart->qdcount); x>0; x--)
  {
    numbytes = dn_skipname(mxcurr, mxend);
    mxcurr += numbytes + QFIXEDSZ;
  }

  // reading all mx-entries
  for (x= ntohs(headstart->ancount); x>0; x--)
  {
    numbytes = dn_expand((UBYTE *)&mxbuffer, mxend, mxcurr, (SBYTE *) mxrec, buflen);
    mxcurr += numbytes;

    GETSHORT(type, mxcurr);
    mxcurr += INT16SZ + INT32SZ;
    GETSHORT(pref, mxcurr);

    if (type != T_MX)
      log.add(3,"error: resolver got invalid type (!T_MX) for domain :%s", domain);

    GETSHORT(pref, mxcurr);

    numbytes = dn_expand((UBYTE *)&mxbuffer, mxend, mxcurr, (SBYTE *) mxrec, buflen);
    mxcurr += numbytes;

    currip = (ULONG) htonl(lookup((SBYTE *) mxrec));
    
    // sorting mx-records and take the best three IP's
    for (y=0; y<3; y++)
    {
      if (pref < mxs[y])
      {
        for (z=2; z>=y; z--)
        {
          mxs[z+1] = mxs[z];
          mip[z+1] = mip[z];
        }
        mxs[y] = pref;
        mip[y] = ntohl(currip);
        break;
      }
    }
  }

  // did'nt get at least one valid MX for domain
  if (!mip[0])
  {
    log.add(3, "error: resolver found no valid IP's for domain: %s", domain);

    // TODO HELP XXXX try to lookup host itself (no MX)
    return(FALSE); 
  }

  // prepare structure
  snprintf(ce.name, MAX_SMTP_DOMAIN_SIZE, "%s", domain);
  ce.cdat = timer.getUnix();
  ce.ip1 = mip[0];
  ce.ip2 = mip[1];
  ce.ip3 = mip[2];

  // opening cache file for writing
  if ((fcache = fopen(FILE_DNS_CACHE, (found == TRUE)?"rb+":"ab")) != NULL)
  {
    if (found == TRUE)
      fseek(fcache, pos, SEEK_SET);

    fwrite(&ce, 1, sizeof(cache_entry), fcache);
    fclose(fcache);
  }
  else
    log.add(1,"error: cannot create/write into cache-file '%s'", FILE_DNS_CACHE);

  *ip1=mip[0];
  *ip2=mip[1];
  *ip3=mip[2];

  return(TRUE); 
}
