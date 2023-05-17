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

#ifndef MESSAGE_H
#define MESSAGE_H

extern Log log;

inline BOOL Message::setName(STRING n)
{
  DEBUGGER("Message::setName()");
  ASSERT(n.size());

  name = n;
  return(TRUE);
}

inline SBYTE * Message::getName(VOID)
{
  DEBUGGER("Message::getName()");
  return((SBYTE *) name.c_str());
}

inline ULONG Message::getSize(VOID)
{
  DEBUGGER("Message::getSize()");

  return(size);
}

inline VOID Message::setSize(ULONG s)
{
  DEBUGGER("Message::setSize()");

  size = s;
}

inline Address * Message::getSender(VOID)
{
  DEBUGGER("Message::getSender()");
  ASSERT(sender);

  return(sender);
}

inline Address * Message::getRec(VOID)
{
  DEBUGGER("Message::getRec()");

  return(currAdd);
}

// adding initial/another recipient for message
inline BOOL Message::addRec(Address * rec)
{
  DEBUGGER("Message::addRec()");
  ASSERT(rec);

  if (!rec)
    return(FALSE);

  recipients.add(rec);
  currAdd = rec;
  return(TRUE);
}

// setting sender of message
inline BOOL Message::setSender(Address * snd)
{
  DEBUGGER("Message::setSender()");
  ASSERT(snd);

  if (!snd)
    return(FALSE);

  sender = snd;
  return(TRUE);
}

#endif
