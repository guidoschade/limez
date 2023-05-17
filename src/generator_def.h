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

#ifndef GENERATORDEF_H
#define GENERATORDEF_H

#include "global.h"

#include "address_def.h"
#include "message_def.h"

class Generator
{
  public:

    Generator();
    ~Generator();

    VOID welcomeBye(SBYTE * fname, Address * rec);
    VOID informAdmin(BOOL add, SBYTE * list, SBYTE * admin, Address * rec);
    VOID preBroadcast(SBYTE * list, ULONG users, Address * rec);
    VOID pastBroadcast(SBYTE * list, ULONG users, ULONG sent, ULONG spooled, ULONG unsubs, ULONG secs, ULONG bytes, Address * rec);
    VOID nonMemberBroadcast(SBYTE * list, SBYTE * policy, SBYTE * admin, Address * rec);
    VOID maxSize(SBYTE * list, Address * rec, ULONG size, SBYTE * admin);
    VOID sendError(Address * rec, SBYTE * admin);

  private:

    inline Message * prepare(Address * rec, BOOL fullheader = TRUE);
    inline VOID finish(Message * msg, Address * rec);
};
#endif
