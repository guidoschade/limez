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

#ifndef TRANSMITTERDEF_H
#define TRANSMITTERDEF_H

#include <unistd.h> // getpid();
#include <signal.h> // kill();
#include <sys/wait.h>

#include "global.h"

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

class Transmitter : public Task
{ 
  public:

    Transmitter();
    ~Transmitter();

    VOID setConn(Connection * conn);
    VOID setMsg(Message * msg);
    VOID setBroadcast(SBYTE * nam);
    BOOL start(VOID);
    BOOL deny(VOID);

  private:

    ULONG rchars, read, wrote, total;
    SBYTE buf[MAX_DATA_SIZE+1];
    STRING listname;
    BOOL broadcast, toList, unknownUser;
    MailingList * ml;

    Connection * conn;
    Address * currAdd;

    inline BOOL readWelcome(VOID);
    inline BOOL writeHelo(VOID);
    inline BOOL writeMail(VOID);
    inline BOOL writeRcpt(VOID);
    inline BOOL writeRset(VOID);
    inline BOOL writeQuit(VOID);
    inline BOOL writeData(VOID);

    VOID sendFile(SBYTE * fname);
    BOOL sendData(VOID);
};

#endif // #ifndef TRANSMITTERDEF_H
