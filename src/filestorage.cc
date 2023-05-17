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

#include "filestorage_def.h"

#include "log_def.h"
#include "line_def.h"
#include "address_inl.h"
#include "keyvalue_inl.h"
#include "datablock_inl.h"

extern Log log;

FileStorage::FileStorage(STORECLASS sclass, FILEMODE fmode)
{
  DEBUGGER("FileStorage::FileStorage()");

  this->fmode = fmode;
  this->sclass = sclass;
  opened = FALSE;
}

FileStorage::~FileStorage()
{
  ASSERT(!opened);
  ASSERT(!file);
}

// opening given file
BOOL FileStorage::open(const SBYTE * fname)
{ 
  DEBUGGER("FileStorage::open()");

  ASSERT(fname);
  ASSERT(!opened);

  strcpy(name, fname);
  line = 0;

  switch(fmode)
  {
    case(MODE_WRITE):
      if ((file = fopen(name, "wb")) == NULL)
      {
        log.add(4,"error: storage could not write file '%s'", name);
        return(FALSE);
      }
      break;

    case(MODE_APPEND):
      if ((file = fopen(name, "ab")) == NULL)
      {
        log.add(4,"error: storage could not write/append file '%s'", name);
        return(FALSE);
      }
      break;

    case(MODE_READ):
      if ((file = fopen(name, "rb")) == NULL)
      {
        log.add(4,"info: storage failed to read file (or lock) '%s'", name);
        return(FALSE);
      }
      break;

    default:
      log.add(1,"error: FileStorage::open() invalid SWITCH");
      return(FALSE);
      break;
  } 
     
  log.add(4,"info: storage opened file '%s' with mode 0x%x", name, fmode);
  opened = TRUE;
  return(TRUE);
}

BOOL FileStorage::close(VOID)
{
  DEBUGGER("FileStorage::close()");

  ASSERT(opened);
  ASSERT(file);

  fclose(file);
  file = NULL;
  opened = FALSE;
  return(TRUE);
}

// remove entries from file
BOOL FileStorage::remove(DataBlock * block)
{
  Storage * tmp = NULL;
  Address * adr = NULL;
  Address * target = NULL;
  STRING    tmpname;
  DataBlock srcblock(TYPE_ADDRESS);
  DataBlock dstblock(TYPE_ADDRESS);

  DEBUGGER("FileStorage::remove()");

  ASSERT(fmode == MODE_READ);
  ASSERT(block);
  ASSERT(opened);
  ASSERT(file);

  switch(block->getType())
  {
    case(TYPE_ADDRESS):
    {
      ULONG num = 0;

      block->get(&target);
      ASSERT(target);

      // create a temp-file and copy all users except the one to remove
      tmp = new FileStorage(STORE_USER, MODE_WRITE);
      ASSERT(tmp);

      tmpname = string(name) + "-temporary";
      if (tmp->open(tmpname.c_str()) == TRUE)
      {
        // read user from storage and copy them to tmp
        while (get(&srcblock, (ULONG)(num++)*MAX_USERS_ONCE + 1, MAX_USERS_ONCE) == TRUE)
        {
          while (srcblock.get(&adr) == TRUE)
          {
            if (target->compare(adr) == TRUE)
              delete adr;
            else
              dstblock.add(adr);
          }

          if (tmp->add(&dstblock) == FALSE)
          {
            log.add(2,"error: write to '%s' failed", tmpname.c_str());
            break;
          }
        }
      }
      else
      {
        log.add(2,"error: opening storage '%s'", tmpname.c_str());
        delete tmp;
        return(FALSE);
      }

      // renaming and closing tmpfile
      tmp->reName(name);
      tmp->close();
      delete tmp;

      return(TRUE);
      break;
    }

    default:
      log.add(1,"error: FileStorage::remove() invalid SWITCH");
      return(FALSE);
      break;
  }
  return(FALSE);
}
 
BOOL FileStorage::change(DataBlock * block)
{
  DEBUGGER("FileStorage::change()");
                               
  ASSERT(fmode == MODE_WRITE);
  ASSERT(block);
  ASSERT(opened);
  ASSERT(file);
  log.add(1,"error: FileStorage::change() not implemented");
  return(FALSE);
}

// store data of given DataBlock into file
BOOL FileStorage::add(DataBlock * block)
{                 
  ULONG bytes = 0, size = 0;

  DEBUGGER("FileStorage::add()");
  ASSERT(opened);
  ASSERT(fmode == MODE_WRITE  || fmode == MODE_APPEND);
  ASSERT(block);
  ASSERT(file);

  switch(block->getType())
  {
    case(TYPE_LINE):
    {
      Line * bline = NULL;
      DEBUGGER("FileStorage::add():TYPE_LINE");

      while(block->get(&bline) == TRUE)
      {
        size = strlen(bline->str);

        bytes = fwrite(bline->str, 1, size, file);
        if (bytes != size)
        {
          log.add(1,"error: write error on '%s' occured (%ld/%ld bytes)", name, bytes, size);
          return(FALSE);
        }
        delete bline;
      }
      return(TRUE);
      break;
    }

    case(TYPE_RAW):
    {
      RawData * raw = NULL;
      DEBUGGER("FileStorage::add():TYPE_RAW");

      while(block->get(&raw) == TRUE)
      {
        ASSERT(raw);
        bytes = fwrite(raw->data, 1, raw->size, file);
        if (bytes != raw->size)
        {
          log.add(1,"error: write error on '%s' occured (%ld/%ld bytes)",
                    name, bytes, raw->size);
          return(FALSE);
        }
      }
      return(TRUE);
      break;
    }             

    case(TYPE_KEYVAL):
    {
      KeyValue * tmp = NULL;
      SBYTE buffer[MAX_KEY_VAL_LINE_SIZE+1];

      DEBUGGER("FileStorage::add():TYPE_KEYVAL");

      while(block->get(&tmp) == TRUE)
      {
        snprintf(buffer, MAX_KEY_VAL_LINE_SIZE, "%-25s \"%s\"\n", tmp->getKey(), tmp->getVal());
        size = strlen(buffer);
        bytes = fwrite(buffer, 1, size, file);
        if (bytes != size)
        {
          log.add(1,"error: write error on '%s' occured (%ld/%ld bytes)", name, bytes, size);
          return(FALSE);
        }
      }
      return(TRUE);
      break;
    }

    case(TYPE_ADDRESS):
    {
      Address * adr = NULL;
      SBYTE buffer[MAX_KEY_VAL_LINE_SIZE+1];

      DEBUGGER("FileStorage::add():TYPE_ADDRESS");
      
      while(block->get(&adr) == TRUE)
      {
        snprintf(buffer, MAX_KEY_VAL_LINE_SIZE, "%s@%s\n", adr->getUser(), adr->getDomain());

        size = strlen(buffer);
        bytes = fwrite(buffer, 1, size, file);
        if (bytes != size)
        {
          log.add(1,"error: write error on '%s' occured (%ld/%ld bytes)", name, bytes, size);
          return(FALSE);
        }
      }
      return(TRUE);
      break;
    }

    default:
    {
      log.add(1,"error: FileStorage::add() invalid SWITCH");
      return(FALSE);
      break;
    }
  }
}

// reading from file and store data in given DataBlock
BOOL FileStorage::get(DataBlock * block, ULONG start = 1, ULONG count = 0)
{
  ULONG end = (start + count)-1;
  ULONG current = 0;
  ULONG bytes = 0, size = 0;
  BOOL  got = FALSE;

  DEBUGGER("FileStorage::get()");
  ASSERT(start);
  ASSERT(opened);
  ASSERT(fmode == MODE_READ);
  ASSERT(block);
  ASSERT(file);

  log.add(8,"info: FileStorage::get(block, start:%ld, count:%ld, end:%ld)", start, count, end);

  // seek to start of file
  if (count)
    fseek(file, 0, SEEK_SET);

  switch(block->getType())
  {
    case(TYPE_KEYVAL):
    {
      KeyValue * newval = NULL;
      SBYTE * keyptr = NULL, * valptr = NULL, * eptr = NULL;
      DEBUGGER("FileStorage::get():TYPE_KEYVAL");

      while (fgets(buf, MAX_LINE_SIZE, file) != NULL)
      {
        line ++;
        keyptr = buf;

        // end of file reached -> stop reading
        if (feof(file))
          break;

        // ignore line, if first char = '#' or '/' or '\n'
        if ((buf[0] != '#') && (buf[0] !='\n') && (buf[0] !='/'))
        {
          // strip leading whitespaces
          while(*keyptr == 0x20 || *keyptr == 0x09) keyptr++;

          // getting key
          valptr = keyptr;
          while(*valptr != 0x20 && *valptr != 0x09 &&
                *valptr != 0x0d && *valptr != 0x0a)
          {
            if ((*valptr > 0x40) && (*valptr < 0x5B))
              *valptr += 0x20;
            valptr++;
          }

          *valptr = '\0';
          valptr ++;

          if ((strlen(keyptr) >= MAX_KEY_SIZE) || (strlen(keyptr) < 1))
          {
            log.add(1,"error: config '%s' line %ld: invalid keysize, exiting", name, line);
            exit(1);
          }

          // strip leading whitespaces
          while(*valptr == 0x20 || *valptr == 0x09) valptr++;

          // do we use quotes
          if ((*valptr == '\"') || (*valptr == '\''))
            valptr ++;

          eptr = valptr;
          while((*eptr != '\"') && (*eptr != '\'') &&
                (*eptr != 0x0d) && (*eptr != 0x0a))
            eptr ++;
          *eptr = '\0';

          if ((strlen(valptr) >= MAX_VAL_SIZE) || (strlen(valptr) < 1))
          {
            log.add(1,"error: config '%s' line %ld: invalid valuesize, exiting", name, line);
            exit(1);
          }
        }
        else
          continue;

        // only return wanted values
        if (++current >= start)
        {
          //log.add(4,"info: key '%s' = '%s'", keyptr, valptr);
          newval = new KeyValue(keyptr, TYPE_STR, CONF_NONE, valptr);
          ASSERT(newval);
          block->add(newval);
          got = TRUE;
        }
        if (count && (current >= end))
          break;
      }
      return(got);
      break;
    }

    case(TYPE_RAW):
    {            
      RawData * raw = new RawData;
      BOOL end = FALSE;

      DEBUGGER("FileStorage::get():TYPE_RAW");
      ASSERT(raw);

      size = MAX_DATA_SIZE;
      raw->data = new SBYTE[size];
      ASSERT(raw->data);
     
      bytes = fread(raw->data, 1, size, file);
      raw->size = bytes;

      if (feof(file))
        end = TRUE;

      if ((end == FALSE) && (bytes != size))
      {
        log.add(2,"error: read on rawfile '%s' failed (%ld/%ld)", name, bytes, size);
        delete raw;
        return(FALSE);
      }
      block->add(raw);
      return(TRUE);
      break;
    }

    case(TYPE_ADDRESS):
    {
      Address * adr = NULL;
      SBYTE * ret = NULL;
      UWORD size;

      DEBUGGER("FileStorage::get():TYPE_ADDRESS");

      while (fgets(buf, MAX_LINE_SIZE, file) != NULL)
      {
        if (feof(file))
          break;

        // check if line is valid
        if ((size = strlen(buf)) > 0)
        {
          // cut return
          buf[size-1] = '\0';

          // only return wanted values
          if (++current >= start)
          {
            adr = new Address;
            ASSERT(adr);
            if ((ret = adr->setIfValid(buf, FALSE)))
            {
              log.add(2,"error: '%s', address: %s", ret, buf);
              delete adr;
            }
            else
            {
              log.add(7,"info: read address '%s@%s'", adr->getUser(), adr->getDomain());
              got = TRUE;
              block->add(adr);
            }
          }
          if (count && (current >= end))
            break;
        }
        else
          log.add(2,"error: storage '%s', line: %ld contains invalid chars", name, current);
      }
      return(got);
      break;
    }

    case(TYPE_LINE):
    {
      DEBUGGER("FileStorage::get():TYPE_LINE");

      if (fgets(buf, MAX_LINE_SIZE, file) != NULL)
      {
        if (!feof(file))
        {
          block->add(new Line(buf));
          got = TRUE;
        }
      }

      return(got);
      break;
    }

    default:
      log.add(1,"error: FileStorage::get() invalid SWITCH");
      return(FALSE);
      break;
  }
  return(FALSE);
}

// delete current file
BOOL FileStorage::destroy(SBYTE * nam)
{
  DEBUGGER("FileStorage::destroy()");
  ASSERT(nam);
  ASSERT(opened);

  if (unlink(nam) != 0)
  {
    log.add(2,"error: storage is unable to delete file '%s'", nam);
    return(FALSE);
  }
  return(TRUE);
}

// rename (move) current file
BOOL FileStorage::reName(const SBYTE * name)
{
  DEBUGGER("FileStorage::reName()");
  ASSERT(this->name);
  ASSERT(name);

  if (rename(this->name, name) != 0)
  {
    log.add(2,"error: storage is unable to rename file '%s' to '%s'", this->name, name);
    return(FALSE);
  }
  strcpy(this->name, name);
  return(TRUE);
}

// return number of values in storage
ULONG FileStorage::getCount(VOID)
{
  ULONG num = 0;

  DEBUGGER("FileStorage::getCount()");
  ASSERT(name);
  ASSERT(opened);
  ASSERT(fmode == MODE_READ);

  switch(sclass)
  {
    case(STORE_CONF):
    case(STORE_USER):
    case(STORE_SENDER):
    {
      while (fgets(buf, MAX_LINE_SIZE, file) != NULL)
      {
        num ++;

        if (feof(file))
          break;
      }
      break;
    }

    default:
      log.add(1,"error: FileStorage::getCount() invalid SWITCH");
      return(0);
      break;
  }

  return(num);
}

// find specified value in storage
BOOL FileStorage::find(DataBlock * block)
{
  BOOL found = FALSE;
  ULONG num = 0;

  DEBUGGER("FileStorage::find()");
  ASSERT(opened);
  ASSERT(fmode == MODE_READ);
  ASSERT(block);

  switch(block->getType())
  {
    case(TYPE_ADDRESS):
    {
      DataBlock sblock(TYPE_ADDRESS);
      Address * adr = NULL, * tmp = NULL;

      DEBUGGER("FileStorage::find():TYPE_ADDRESS");

      if (block->get(&adr) == TRUE)
      {
        while (get(&sblock, (ULONG)(num++)*MAX_USERS_ONCE + 1, MAX_USERS_ONCE) == TRUE)
        {
          while (sblock.get(&tmp) == TRUE)
          {
            if (adr->compare(tmp) == TRUE)
            {
              delete tmp;
              found = TRUE;
              break;
            }
            delete tmp;
          }
        }
      }
      break;
    }

    default:
      log.add(1,"error: FileStorage::find() invalid SWITCH");
      return(FALSE);
      break;
  }
  return(found);
}

// copy contents of other storage to current
BOOL FileStorage::clone(const SBYTE * newname)
{
  FileStorage * tmp = NULL;
  DataBlock srcblock(TYPE_ADDRESS);
  DataBlock dstblock(TYPE_ADDRESS);
  Address * adr = NULL;

  DEBUGGER("FileStorage::clone()");
  ASSERT(newname);
  ASSERT(opened);

  switch(sclass)
  {
    case(STORE_USER):
    {
      ULONG num = 0;

      // create tmp storage
      tmp = new FileStorage(STORE_USER, MODE_WRITE);
      if (tmp->open(newname) == TRUE)
      {
        // read user from storage and copy them to tmp
        while (get(&srcblock, (ULONG)(num++)*MAX_USERS_ONCE + 1, MAX_USERS_ONCE) == TRUE)
        {
          while (srcblock.get(&adr) == TRUE)
            dstblock.add(adr);

          if (tmp->add(&dstblock) == FALSE)
          {
            log.add(2,"error: write to '%s' failed", newname);
            break;
          }
        }
      }
      else
      {
        log.add(2,"error: opening storage '%s'", newname);
        delete tmp;
        return(FALSE);
      }

      tmp->close();
      delete tmp;
      return(TRUE);
      break;
    }

    default:
      log.add(1,"error: FileStorage::clone() invalid SWITCH");
      return(FALSE);
      break;
  }

  return(TRUE);
}
