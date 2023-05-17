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

#ifndef CONNECTION_H
#define CONNECTION_H

extern Log log;

// returning status of connection
inline UBYTE Connection::getStatus(VOID)
{
  DEBUGGER("Connection::getStatus()");

  return(status);
}

// reading from socket and returning num of bytes
inline ssize_t Connection::cread(SBYTE * buf, ULONG size = MAX_TRANS_SIZE)
{
  INT res;
  SBYTE c;
  ULONG len = 0;
  BOOL ret = FALSE, read = FALSE;

  DEBUGGER("Connection::cread()");
  ASSERT(status == CONN_OPEN);

  if (waitForData() == FALSE)
    return(0);

  // get only one line (ugly hack, but the other stuff did not work)
  while(TRUE)
  {
    res = recv(newsock, (SBYTE *)&c, 1, 0);

    if (res < 0)
    {
      log.add(3,"error: recv failed ('%s')\n", strerror(errno));
      return(0);
    }

    if (res == 0)
    {
      if (read == FALSE)
      {
        log.add(2,"error: recv failed, got empty line\n");
        return(0);
      }
      break;
    }

    read = TRUE;
    *(buf+len) = c;

    if (++len >= size)
      break;

    if (c == '\n')
      if (ret == TRUE)
        break;

    if (c == '\r')
      ret = TRUE;
    else
      ret = FALSE;
  }

  // terminate string
  buf[len]='\0';

  if (len < 132)
    log.add(5,"info: read: %s", buf);
  else
    log.add(5,"info: read %d bytes data from remote\n", len);

  return(len);
}

// writing to socket and return sent bytes
inline ssize_t Connection::cwrite(SBYTE * buf)
{
  ssize_t len;
  INT size;

  DEBUGGER("Connection::cwrite()");
  ASSERT(status == CONN_OPEN);

  // HELP TODO XXX wait (block) / timeout !!!
  size = strlen(buf);
  if ((len = send(newsock, buf, size, 0)) != size)
  {
    log.add(3,"error: write to socket failed (len = %ld)\n", len);
    errors++;
    return(FALSE);
  }

  if (size < 132)
    log.add(5,"info: sent: %s", buf);
  else
    log.add(5,"info: sent %d bytes data to remote\n", size);

  return(len);
}

// reading line from socket
inline ULONG Connection::rawRead(RawData * raw)
{
  DEBUGGER("Connection::rawRead()");
  ASSERT(status == CONN_OPEN);
  ASSERT(raw);

  raw->size = cread(raw->data, raw->size);
  return(raw->size);
}

// on nonblocking socket wait until timeout applies
inline BOOL Connection::waitForData(VOID)
{
  struct timeval to;
  fd_set fd;
  INT res;

  DEBUGGER("Connection::waitForData()");
  ASSERT(status == CONN_OPEN);

  FD_ZERO(&fd);
  FD_SET(newsock, &fd);

  to.tv_sec = timeout;
  to.tv_usec = 0;

  while(TRUE)
  {
    if ((res = select(newsock + 1, &fd, NULL, NULL, &to)) == 0)
    {
      log.add(2,"info: timeout (%d secs) on socket occured while receiving\n", timeout);
      errors ++;
      return(FALSE);
    }

    if (res >= 0)
      break;
  }
  return(TRUE);
}

// return number of errors to task
inline INT Connection::getErrors(VOID)
{
  DEBUGGER("Connection::getErrors()");
  return(errors);
}

#endif
