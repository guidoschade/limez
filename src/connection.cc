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

#include "connection_def.h"
#include "connection_inl.h"

extern Config config;
extern Log log;
extern Resolver dns;

Connection::Connection()
{
  localip = remoteip = 0;
  status = CONN_INIT;
  errors = sock = 0;
  timeout = 120;
  backlog = 5;
  on = 1;

  localips = "0.0.0.0";
  remoteips = localips;
  localhost = "[not set]";
  remotehost = localhost;
}

Connection::~Connection()
{
}

// listen on given interface
BOOL Connection::prepare()
{
  SBYTE * lnam = NULL, * lip = NULL;
  SBYTE * lhosts = config.getValue(KEY_SERVER_HOST);

  DEBUGGER("Connection::prepare()");
  ASSERT(status == CONN_INIT);

  log.add(9,"Connection::prepare() setting timeouts");
  timeout = config.getIntValue(KEY_SERVER_TIMEOUT);

  log.add(9,"Connection::prepare() setting backlog");
  backlog = config.getIntValue(KEY_SERVER_BACKLOG);

  log.add(9,"Connection::prepare() creating socket");

  // create socket
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock<0)
  {
    log.add(1,"error: create socket (system call) failed (%s)", strerror(errno));
    return(FALSE);
  }

  log.add(9,"Connection::prepare() setting sockopts");

  // important for connect to same port again later
  if (setsockopt(sock, SOL_SOCKET,SO_REUSEADDR, &on, sizeof(on)))
  {
    log.add(1,"error: setsockopt (SO_REUSEADDR) failed (%s)", strerror(errno));
    return(FALSE);
  }

  local.sin_family = AF_INET;
  local.sin_port = htons((UWORD) config.getIntValue(KEY_SERVER_PORT));

  if (lhosts)
    local.sin_addr.s_addr = htonl(dns.lookup(lhosts));

  log.add(9,"Connection::prepare() binding socket");

  // binding socket (assigning a name)
  size = sizeof(local);
  if (bind(sock, (struct sockaddr *) &local, size))
  {
    log.add(1,"error: bind (system call) to %s:%ld failed (%s)",
               lhosts?lhosts:"*", config.getIntValue(KEY_SERVER_PORT), strerror(errno));
    return(FALSE);
  }
 
  log.add(9,"Connection::prepare() getting socket name");

  // get local connection structure
  size = sizeof(local);
  if (getsockname(sock, (struct sockaddr *) &local, &size) <0)
  {
    log.add(1,"error: getsockname call failed (%s)", strerror(errno));
    return(FALSE);
  }

  listenport = ntohs(local.sin_port);
  ASSERT(listenport);

  log.add(9,"Connection::prepare() starting listen");

  // just listen
  if (listen (sock, backlog))
  {
    log.add(1,"error: system call 'listen' failed (%s)", strerror(errno));
    return(FALSE);
  }

  log.add(9,"Connection::prepare() getting localip");
  if (lhosts)
    localip = ntohl(local.sin_addr.s_addr);

  if (lhosts && !localip)
  { 
    log.add(1,"error: failed to get ip/hostname for '%s'", lhosts);
    return(FALSE);
  }

  log.add(9,"Connection::prepare() listen port = %ld", listenport);
  log.add(9,"Connection::prepare() local ip = 0x%lx", lhosts?localip:0);

  if (lhosts)
  {
    lip = dns.iptos(localip);
    ASSERT(lip);
    log.add(9,"Connection::prepare() local ip = '%s'", lip);

    lnam = dns.reverse(htonl(localip));
    ASSERT(lnam);
    log.add(9,"Connection::prepare() local name = '%s'", lnam);
  }
  else
  {
    lip = "*";
    lnam = "all interfaces";
  }

  log.add(1,"info: listening on %s:%ld (%s)", lip, (ULONG) listenport, lnam);

  status = CONN_LIST;
  return(TRUE);
}

// waiting for connection
BOOL Connection::awaiting()
{
  INT flags = 0;

  DEBUGGER("Connection::awaiting()");
  ASSERT(status == CONN_LIST);

  size = sizeof(remote);
  log.add(9,"Connection::awaiting() accept");

  // accepting
  while(TRUE)
  {
    newsock = accept(sock, (struct sockaddr *) &remote, &size );
    if (newsock == -1)
    {
      // interrupted system call
      if (errno == 4)
        continue;
  
      log.add(3,"error: system call 'accept' failed");
      return(FALSE);
    }
    break;
  }

  // setting keepalive
  if (setsockopt(newsock, SOL_SOCKET,SO_KEEPALIVE, &on, sizeof(on)))
  {
    log.add(1,"error: setsockopt (SO_KEEPALIVE) failed (%s)", strerror(errno));
    return(FALSE);
  }

  // setting nodelay
  if (setsockopt(newsock, IPPROTO_TCP,TCP_NODELAY, &on, sizeof(on)))
  {
    log.add(1,"error: setsockopt (TCP_NODELAY) failed (%s)", strerror(errno));
    return(FALSE);
  }

  // setting socket nonblocking
  flags = fcntl(newsock, F_GETFL);
  if (fcntl(newsock, F_SETFL, flags|O_NONBLOCK))
  {
    log.add(1,"error: fcntl(O_NONBLOCK) failed (%s)", strerror(errno));
    return(FALSE);
  }

  // getting local connection structure
  size = sizeof(local);
  log.add(9,"Connection::awaiting() getting local sockname");
  if (getsockname(newsock, (struct sockaddr *) &local, &size))
  {
    log.add(1,"error: getsockname (local) failed (%s)", strerror(errno));
    return(FALSE);
  }

  log.add(9,"Connection::awaiting() getting remoteip");
  remoteip = ntohl(remote.sin_addr.s_addr);

  log.add(9,"Connection::awaiting() getting localip");
  localip = ntohl(local.sin_addr.s_addr);

  log.add(9,"Connection::awaiting() looking up localhost");
  localhost = dns.reverse(htonl(localip));

  log.add(9,"Connection::awaiting() looking up remotehost");
  remotehost = dns.reverse(htonl(remoteip));

  localips = dns.iptos(localip);
  remoteips = dns.iptos(remoteip);

  log.add(3,"info: incoming connection [%s] (%s) --> [%s] (%s)",
    remoteips.c_str(), remotehost.c_str(), localips.c_str(), localhost.c_str());

  status = CONN_OPEN;
  return(TRUE);
}

// trying to establish a new tcp-connection
BOOL Connection::copen(ULONG remip, UWORD port, ULONG locip)
{
  DEBUGGER("Connection::copen()");

  remote.sin_family      = AF_INET;
  remote.sin_port        = htons(port);
  remote.sin_addr.s_addr = htonl(remip);
  local.sin_family       = AF_INET;

  log.add(9,"Connection::prepare() setting timeouts");
  timeout = config.getIntValue(KEY_SERVER_TIMEOUT);

  if (locip)
  {
    local.sin_addr.s_addr  = htonl(locip);
    log.add(6,"info: localip 0x%lx", local.sin_addr.s_addr);

    log.add(9,"Connection::copen() getting localip");
    localip = ntohl(local.sin_addr.s_addr);
    localips = dns.iptos(localip);

    // check if both ip's and ports are equal
    if (locip == remip && port == config.getIntValue(KEY_SERVER_PORT))
    {
      log.add(3,"error: refuse to play with myself (IP %s, same port), trying next MX", localips.c_str());
      errors ++;
      return(FALSE);
    }
  }

  log.add(6,"info: remoteip 0x%lx", remote.sin_addr.s_addr);
  log.add(9,"Connection::copen() getting remoteip");
  remoteip = ntohl(remote.sin_addr.s_addr);
  remoteips = dns.iptos(remoteip);

  log.add(9,"Connection::copen() looking up remotehost");
  remotehost = dns.reverse(htonl(remoteip));

  log.add(3,"info: connecting [%s] --> [%s:%ld] (%s)", locip?localips.c_str():"auto",
            remoteips.c_str(), port, remotehost.c_str());

  // create socket
  if ((newsock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    log.add(3,"error: getting socket (system call) failed (%s)", strerror(errno));
    errors ++;
    return(FALSE);
  }

  // binding on local interface if set
  if (locip && bind(newsock, (struct sockaddr *) &local, sizeof(local)))
  {
    log.add(4,"error: bind call to local address (%s) failed (%s)",
      dns.iptos(ntohl(local.sin_addr.s_addr)), strerror(errno));
    errors++;
  }

  // trying to connect
  if (connect(newsock, (struct sockaddr *) &remote, sizeof(struct sockaddr_in)))
  {
    log.add(3,"error: socket connect call failed (%s)", strerror(errno));
    errors ++;
    return(FALSE);
  }

  // getting local connection structure
  size = sizeof(local);
  log.add(9,"Connection::awaiting() getting local sockname");
  if (getsockname(newsock, (struct sockaddr *) &local, &size))
  {
    log.add(1,"error: getsockname (local) failed (%s)", strerror(errno));
    return(FALSE);
  }

  if (!locip)
  {
    log.add(9,"Connection::copen() getting localip");
    localip = ntohl(local.sin_addr.s_addr);
    localips = dns.iptos(localip);

    log.add(9,"Connection::copen() looking up localhost");
    localhost = dns.reverse(htonl(localip));
  }

  log.add(3,"info: connected with ip [%s] to [%s:%ld]",
            localips.c_str(), remoteips.c_str(), port);

  status = CONN_OPEN;
  return(TRUE);
}

// trying to shutdown a tcp-connection (client socket)
BOOL Connection::cclose(BOOL shut)
{
  DEBUGGER("Connection::cclose()");
  ASSERT(status > 0);

  if (shut == TRUE)
  {
    log.add(9,"Connection::cclose() shutdown");

    // shutting down socket
    if (shutdown(newsock,0) == -1)
    {
      log.add(3,"error: shutdown of socket (system call) failed");
      errors++;
      status = CONN_LIST;
      return(FALSE);
    }
  }

  log.add(9,"Connection::cclose() close");

  if (close(newsock))
  {
    log.add(3,"error: close of socket (system call) failed");
    errors++;
    status = CONN_LIST;
    return(FALSE);
  }
  
  if (shut == TRUE)
    log.add(3,"info: connection to [%s] closed", remoteips.c_str());
 
  status = CONN_LIST;
  return(TRUE);
}

// shutdown server socket we were listening on
BOOL Connection::sclose(VOID)
{
  DEBUGGER("Connection::sclose()");

  if (close(sock))
  {
    log.add(3,"error: close of socket (system call) failed");
    return(FALSE);
  }

  return(TRUE);
}
