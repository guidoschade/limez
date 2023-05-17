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

#ifndef RESOLVERDEF_H 
#define RESOLVERDEF_H

#include <sys/types.h>
#include <netinet/in.h>
#include <resolv.h>

#include <arpa/nameser.h>
#include <netdb.h>
#include <errno.h>

#include "log_def.h"
#include "config_def.h"
#include "timer_def.h"

#include "global.h"

class Resolver
{ 
  public:

     Resolver();
     ~Resolver();

    SBYTE * reverse(ULONG ip);
    SBYTE * iptos(ULONG ip);
    ULONG lookup(SBYTE * name);
    BOOL  getExchanger(SBYTE * domain, ULONG * ip1, ULONG * ip2, ULONG * ip3);

  private:

     UBYTE mxbuffer[MAX_PACKET_SIZE+1];

};

#endif // RESOLVERDEF_H
