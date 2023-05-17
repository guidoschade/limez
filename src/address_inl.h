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

#ifndef ADDRESSINL_H 
#define ADDRESSINL_H

inline BOOL Address::setUser(SBYTE * nm)
{
  if (nm)
  {
    username = nm;
    return(TRUE);
  }
  return(FALSE);
}

inline BOOL Address::setDomain(SBYTE * dom)
{
  if (dom)
  {
    domain = dom;
    return(TRUE);
  }
  return(FALSE);
}

inline SBYTE * Address::getUser(VOID)
{
  return((SBYTE *) username.c_str());
}

inline SBYTE * Address::getDomain(VOID)
{
  return((SBYTE *) domain.c_str());
}

#endif
