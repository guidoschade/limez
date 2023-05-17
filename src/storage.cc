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

#include "storage_def.h"

Storage::Storage()
{
}

Storage::~Storage()
{
}

BOOL Storage::open(const SBYTE * name)
{
  return(TRUE);
}

BOOL Storage::close(VOID)
{
  return(TRUE);
}

BOOL Storage::add(DataBlock * block)
{
  return(TRUE);
}

BOOL Storage::find(DataBlock * block)
{
  return(TRUE);
}

BOOL Storage::get(DataBlock * block, ULONG start = 1, ULONG count = 0)
{
  return(TRUE);
}
 
BOOL Storage::remove(DataBlock * block)
{
  return(TRUE);
}

BOOL Storage::change(DataBlock * block)
{
  return(TRUE);
}

BOOL Storage::destroy(SBYTE * nam)
{
  return(TRUE);
}

BOOL Storage::reName(const SBYTE * newname)
{
  return(TRUE);
}

BOOL Storage::clone(const SBYTE * newname)
{
  return(TRUE);
}

ULONG Storage::getCount(VOID)
{
  return(0);
}
