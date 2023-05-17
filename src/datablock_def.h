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

#ifndef DATABLOCKDEF_H 
#define DATABLOCKDEF_H

#include "global.h"
#include "listelement_def.h"
#include "line_def.h"
#include "address_def.h"
#include "keyvalue_def.h"
#include "rawdata_def.h"

#include "listelement_inl.h"

class DataBlock
{ 
  public:

    DataBlock(BLOCKTYPE btype);
    ~DataBlock(); 

    VOID clear();           

    inline BOOL get(Line ** tmp);
    inline BOOL get(RawData ** tmp);
    inline BOOL get(Address ** tmp);
    inline BOOL get(KeyValue ** tmp);

    inline BOOL add(Line * tmp);
    inline BOOL add(RawData * tmp);
    inline BOOL add(Address * tmp);
    inline BOOL add(KeyValue * tmp);

    inline ULONG getCount(VOID);

    inline BLOCKTYPE getType(VOID);

  private:
    
    List<Line> lines;
    List<Address> addresses;
    List<KeyValue> keyvals;
    List<RawData> rawdata;
     
    BLOCKTYPE btype;
};

#endif // #ifndef DATABLOCKDEF_H
