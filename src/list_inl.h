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

#ifndef LIST_H 
#define LIST_H

#include "list_def.h"
#include "listelement_def.h"

template <class T>
inline List<T>::List()
: first(NULL), last(NULL), count(0)
{
}

template <class T>
inline List<T>::~List()
{
  clear();
}

template <class T>
inline T * List<T>::getFirst()
{
  return last;
}

template <class T>
inline T * List<T>::getLast()
{
  return first;
}

template <class T>
inline T * List<T>::getNext(T * le)
{
  ASSERT(le);
  return le->prev;
}

template <class T>
inline T * List<T>::getPrev(T * le)
{
  ASSERT(le);
  return le->next;
}

template <class T>
inline VOID List<T>::add(T * le)
{
  ASSERT(le);
  ASSERT(!le->list);

  if (first)
    first->prev = le;
  else
    last = le;

  le->next = first;
  first = le;

  le->prev = NULL;
  le->list = this;
  count++;
}

template <class T>
inline VOID List<T>::del(T * le)
{
  ASSERT(le);

  if (le == last)
    if (le->prev)
      last = le->prev;
    else
      last = NULL;

  le->unhook();
}

template <class T>
inline VOID List<T>::clear(VOID)
{
  T * o, * c = first;
  
  while (c)
  {
    o = c;
    c = o->next;
    o->unhook(); 
    delete o;
  }
  count = 0;
  first = last = NULL;
}

template <class T>
inline ULONG List<T>::getCount()
{
  return count;
}

#endif // LIST_H
