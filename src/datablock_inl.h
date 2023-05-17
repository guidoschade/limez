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

#ifndef DATABLOCK_H
#define DATABLOCK_H

extern Log log;

// return content - type of block
inline BLOCKTYPE DataBlock::getType(VOID)
{
  DEBUGGER("DataBlock::getType()");

  return(btype);
}

// return the number of units the block contains
inline ULONG DataBlock::getCount(VOID)
{
  ULONG num = 0;
  DEBUGGER("DataBlock::getCount()");

  switch(btype)
  {
    case(TYPE_LINE):
      return(lines.getCount());
      break;

    case(TYPE_ADDRESS):
      return(addresses.getCount());
      break;

    case(TYPE_KEYVAL):
      return(keyvals.getCount());
      break;

    case(TYPE_RAW):
      return(rawdata.getCount());
      break;

    default:
      ASSERT(FALSE);
      break;
  }

  return(num);
}

// get next line and remove it from block
inline BOOL DataBlock::get(Line ** tmp)
{
  DEBUGGER("DataBlock::get(Line *)");
  ASSERT(btype == TYPE_LINE);
  *tmp = NULL;

  if (lines.getCount())
  {
    *tmp = lines.getFirst();
    ASSERT(*tmp);
    lines.del(*tmp);
    return(TRUE);
  }
  return(FALSE);
}
  
// get next keyvalue and remove it from block
inline BOOL DataBlock::get(KeyValue ** tmp)
{
  DEBUGGER("DataBlock::get(KeyValue *)");
  ASSERT(btype == TYPE_KEYVAL);
  *tmp = NULL;

  if (keyvals.getCount())
  {
    *tmp = keyvals.getFirst();
    ASSERT(*tmp);
    keyvals.del(*tmp);
    return(TRUE);
  }
  return(FALSE);
}
      
// get next rawdata and remove it from block
inline BOOL DataBlock::get(RawData ** tmp)
{
  DEBUGGER("DataBlock::get(RawData *)");
  ASSERT(btype == TYPE_RAW);
  *tmp = NULL;

  if (rawdata.getCount())
  {
    *tmp = rawdata.getFirst();
    ASSERT(*tmp);
    rawdata.del(*tmp);
    return(TRUE);
  }
  return(FALSE);
}

// get next address and remove it from block
inline BOOL DataBlock::get(Address ** tmp)
{ 
  DEBUGGER("DataBlock::get(Address *)");
  ASSERT(btype == TYPE_ADDRESS);
  *tmp = NULL;

  if (addresses.getCount())
  {
    *tmp = addresses.getFirst();
    ASSERT(*tmp);
    addresses.del(*tmp);
    return(TRUE);
  }
  return(FALSE);
}

// add new line to block
inline BOOL DataBlock::add(Line * tmp)
{
  DEBUGGER("DataBlock::add(Line)");
  ASSERT(tmp);
  ASSERT(btype == TYPE_LINE);
  
  lines.add(tmp);    
  return(TRUE);
}

// add new address to block
inline BOOL DataBlock::add(Address * tmp)
{
  DEBUGGER("DataBlock::add(Address)");
  ASSERT(tmp);
  ASSERT(btype == TYPE_ADDRESS);
  
  addresses.add(tmp);    
  return(TRUE);
}

// add new keyvalue to block
inline BOOL DataBlock::add(KeyValue * tmp)
{
  DEBUGGER("DataBlock::add(KeyValue)");

  ASSERT(tmp);
  ASSERT(btype == TYPE_KEYVAL);
  
  keyvals.add(tmp);
  return(TRUE);
}

// add new rawdata to block
inline BOOL DataBlock::add(RawData * tmp)
{
  DEBUGGER("DataBlock::add(RawData)");

  ASSERT(tmp);
  ASSERT(btype == TYPE_RAW);
  
  rawdata.add(tmp);    
  return(TRUE);
}

// delete all values from block
inline VOID DataBlock::clear(VOID)
{
  switch(btype)
  {
    case(TYPE_LINE): 
      lines.clear();
      break;

    case(TYPE_KEYVAL): 
      keyvals.clear();
      break;

    case(TYPE_ADDRESS):
      addresses.clear(); 
      break;

    case(TYPE_RAW): 
      rawdata.clear();
      break;

    default:
      ASSERT(FALSE);
      break;
  }
}
#endif
