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

#ifndef MAILINGLISTDEF_H 
#define MAILINGLISTDEF_H 

#include "global.h"

#include "config_def.h"
#include "list_def.h"
#include "log_def.h"
#include "address_def.h"

class Config;

class MailingList: public ListElement<MailingList>
{ 
  public:

    MailingList();
    ~MailingList();

    Log    mllog;
    Config mlconfig;

    BOOL  initConfig(SBYTE * clist, BOOL db = FALSE);
    BOOL  remove(VOID);

    BOOL  getAllSender(DataBlock * b);

    BOOL  getAllUsers(DataBlock * b, ULONG num, ULONG count = MAX_USERS_ONCE, BOOL temp = FALSE);
    ULONG getUserCount(VOID);

    BOOL  isUserMember(Address * rec);
    BOOL  isUserSender(Address * rec);
    BOOL  isUserAuthorized(Address * usr);

    BOOL  addUser(Address * rec);
    BOOL  delUser(Address * rec);

    BOOL  addSender(Address * rec);
    BOOL  delSender(Address * rec);
};

#endif // #ifndef MAILINGLISTDEF_H
