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

#ifndef LIGHTDIRDEF_H
#define LIGHTDIRDEF_H

#include "global.h"
#include "keyvalue_def.h"

#ifdef USE_LDAP
  #include "ldap.h"
#endif

class LightDir
{
  public:

    LightDir();
    ~LightDir();

    VOID disconnect(VOID);
    VOID clearResults(VOID);

    BOOL connect(SBYTE * ldaphost, SBYTE * ldapuser, SBYTE * ldappass);
    BOOL search(SBYTE * base, BOOL deep = TRUE, SBYTE * filter = NULL, SBYTE ** attrs = NULL);
    KeyValue ** getEntry(VOID);

  private:

#ifdef USE_LDAP
    LDAP         * ldapsock;
    LDAPMessage  * result, * entry;
    BerElement   * ber;
    KeyValue    ** resptr;
#endif

    BOOL  connected;
    BOOL  gotresults;
    INT   maxValues;
};

#endif
