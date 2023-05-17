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

#ifndef MESSAGEDEF_H 
#define MESSAGEDEF_H 

#include <stdlib.h>

#include "global.h"

#include "config_def.h"
#include "address_def.h"
#include "mailinglist_def.h"
#include "list_def.h"
#include "line_def.h"
#include "log_def.h"

#include "address_inl.h"
#include "log_inl.h"

class MailingList;

class Message: public ListElement<Message>
{ 
  public:

    Message();
    ~Message();

    VOID init(VOID);
    VOID addLine(const SBYTE * ...);

    BOOL queue(SBYTE * stat, QOPTIONS opts);
    BOOL unqueue(STRING conf);

    BOOL lock(VOID);
    BOOL unlock(const SBYTE * lnam = NULL);
    BOOL hasLock(VOID);
    BOOL save(VOID);
    BOOL moveToDone(SBYTE * done);
    Message * clone(const SBYTE * lname, Address * nrec = NULL);

    BOOL generateAnswer(SBYTE * srcname);
    BOOL parseCommand(BOOL approved, BOOL quote, SBYTE * cmd);

    List<Line> lines;
    List<Address> recipients;

    inline BOOL addRec(Address * rec);
    inline BOOL setSender(Address * snd);

    inline BOOL  setName(STRING n);
    inline SBYTE * getName(VOID);

    inline VOID  setSize(ULONG s);
    inline ULONG getSize(VOID);

    inline Address * getSender(VOID);
    inline Address * getRec(VOID);

    Config * mconf;
    Log mlog;

  private:

    MailingList * ml;
    Address * sender, * currAdd;
    Line * currLine;

    STRING name;
    ULONG  size, tries;
    UWORD  errors;
    BOOL   quiet;

    BOOL change(BOOL approved, BOOL option, CMDTYPE type, SBYTE * cname, SBYTE * cmd);
    BOOL changeuser(BOOL approved, BOOL option, CMDTYPE type, Address * rec);
    BOOL changesender(BOOL approved, BOOL option, CMDTYPE type, Address * rec);

    BOOL approve(SBYTE * cname, SBYTE * pass);
};

#endif // MESSAGEDEF_H
