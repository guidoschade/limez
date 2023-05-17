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

#ifndef CONNECTIONDEF_H 
#define CONNECTIONDEF_H

#include "global.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <fcntl.h>

#include "resolver_def.h"
#include "log_def.h"
#include "config_def.h"

class Connection
{ 
  friend class Transmitter;
  friend class Receiver;

  private:

    INT  on, sock, backlog, newsock, listenport, errors, timeout;
    struct sockaddr_in local, remote;
    size_t size;

    CONNTYPE  status;
    inline BOOL waitForData(VOID);

  public:

    Connection();
    ~Connection();  

    BOOL prepare(VOID);
    BOOL awaiting(VOID);
    BOOL copen(ULONG remip, UWORD port, ULONG locip);
    BOOL cclose(BOOL shut);
    BOOL sclose(VOID);
    UBYTE getStatus(VOID);

    inline INT getErrors(VOID);
    inline ssize_t cread(SBYTE * buf, ULONG size = MAX_TRANS_SIZE);
    inline ssize_t cwrite(SBYTE * buf);
    inline ULONG rawRead(RawData * raw);

  protected:

    ULONG  localip, remoteip;
    STRING localips, remoteips, localhost, remotehost;
};

#endif // CONNECTIONDEF_H
