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

#ifndef LISTELEMENT_H
#define LISTELEMENT_H

#include "global.h"
#include "listelement_def.h"

template<class T>
inline ListElement<T>::ListElement()
: next(NULL), prev(NULL), list(NULL)
{
}

template<class T>
inline ListElement<T>::~ListElement()
{
  unhook();
}

template<class T>
inline VOID ListElement<T>::unhook()
{
  if (list)
  {
    if(list->first == this)
    {
      list->first = next;
    }

    list->count--;
    list = NULL;

    if (prev)
      prev->next = next;
    if (next)
      next->prev = prev;

    next = prev = NULL;
  }
}

#endif //LISTELEMENT_H
