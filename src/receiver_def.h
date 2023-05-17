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

#ifndef RECEIVERDEF_H
#define RECEIVERDEF_H

#include "global.h"

#include <sys/wait.h>
#include <unistd.h> // getpid();
#include <signal.h> // kill();

#include "log_def.h"
#include "task_def.h"
#include "connection_def.h"
#include "timer_def.h"
#include "address_def.h"
#include "list_def.h"
#include "message_def.h"
#include "storage_def.h"
#include "filestorage_def.h"

#include "list_inl.h"
#include "address_inl.h"
#include "message_inl.h"

class Receiver : public Task
{ 
  public:

    Receiver();
    Receiver(Connection * conn);
    ~Receiver();

    VOID setConn(Connection * conn);
    BOOL start(VOID);
    BOOL deny(VOID);

  private:

    BOOL authorized;
    CONSTATE status;
    ULONG rchars; //, errors;
    SBYTE buf[MAX_DATA_SIZE+1];

    Connection * conn;
    Address * currAdd;

    inline BOOL writeWelcome(VOID);
    inline BOOL readQuit(VOID);
    inline BOOL readLadd(VOID);
    inline BOOL readLdel(VOID);
    inline BOOL readLpwd(VOID);
    inline BOOL readLrld(VOID);
    inline BOOL readLcmd(VOID);
    inline BOOL readLshw(VOID);
    inline BOOL readRset(VOID);
    inline BOOL readHelo(VOID);
    inline BOOL readMail(VOID);
    inline BOOL readRcpt(VOID);

    BOOL readData(VOID);
};

#endif // #ifndef RECEIVERDEF_H
