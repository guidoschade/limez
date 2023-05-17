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

#ifndef ADDRESSDEF_H
#define ADDRESSDEF_H

#include "global.h"

#include "listelement_def.h"
#include "listelement_inl.h"
#include "list_inl.h"

class Address : public ListElement<Address>
{
  public:

    Address();
    ~Address();  
    
    inline SBYTE * getUser(VOID);
    inline SBYTE * getDomain(VOID);

    RECTYPE  isValidRec(BOOL checkdomain = TRUE);
    BOOL     compare(Address * adr);
    SBYTE  * setIfValid(SBYTE * mail, BOOL resolve = TRUE);
    SBYTE  * setUserName(SBYTE * uname);
    SBYTE  * setDomainName(SBYTE * udomain);

    ULONG  mip[3];

  private:

    inline BOOL  setUser(SBYTE * nm);
    inline BOOL  setDomain(SBYTE * dom);
    
    STRING firstname;
    STRING lastname;
    STRING title;
    STRING username;
    STRING domain;
};

#endif // ADDRESSDEF_H
