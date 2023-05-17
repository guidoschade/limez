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

#include "transmitter_def.h"
#include "datablock_inl.h"
#include "connection_inl.h"

#include "scheduler_def.h"
#include "scheduler_inl.h"

extern Log log;
extern Timer timer;
extern Config  config;
extern Scheduler scheduler;
extern Resolver dns;
extern List<MailingList> lists;
extern Task * task;

static VOID trans_signal(INT sig);
extern MailingList * getList(const SBYTE * listname, const SBYTE * listdomain = NULL);
extern SBYTE * NOACCEPT;

Transmitter::Transmitter(VOID)
{
  rchars = read = wrote = total = errors = 0;
  currMsg = NULL;
  currAdd = NULL;
  conn = NULL;
  listname = "";
  broadcast = FALSE;
  toList = FALSE;
  unknownUser = FALSE;
  ml = NULL;
}

Transmitter::~Transmitter()
{
}

VOID Transmitter::setMsg(Message * msg)
{
  DEBUGGER("Transmitter::setMsg()");
  ASSERT(msg);

  currMsg = msg;
}

VOID Transmitter::setConn(Connection * conn)
{
  DEBUGGER("Transmitter::setConn()");
  ASSERT(conn);

  this->conn = conn; 
}

VOID Transmitter::setBroadcast(SBYTE * nam)
{
  DEBUGGER("Transmitter::setListName()");
  ASSERT(nam);

  listname = nam;
  broadcast = TRUE;
}

// send e-mail
BOOL Transmitter::start(VOID)
{
  Address * rec = NULL, * snd = NULL;
  Message * msg = NULL;
  BOOL successful = FALSE;
  UBYTE num = 0;

  SBYTE * sendhosts = config.getValue(KEY_SERVER_SEND_HOST);
  BOOL testconnect = config.getBoolValue(KEY_DEBUG_CONNECT_ONLY);
  ULONG sendhost = 0, remport = config.getIntValue(KEY_SERVER_REMOTE_PORT);

  DEBUGGER("Transmitter::start()");

  // installing signal handlers
  sigINT  = new SignalHandler(SIGINT, trans_signal);
  sigHUP  = new SignalHandler(SIGHUP, trans_signal);
  sigPIPE = new SignalHandler(SIGPIPE, trans_signal);
  sigCHLD = new SignalHandler(SIGCHLD, trans_signal);
  sigALRM = new SignalHandler(SIGALRM, trans_signal);

  ASSERT(currMsg);
  ASSERT(currMsg->recipients.getCount() == 1);

  rec = currMsg->getRec();
  snd = currMsg->getSender();

  ASSERT(snd);
  ASSERT(rec);

  // check if message is broadcast of list or generated
  if (broadcast == TRUE && (ml = getList(listname.c_str())))
    toList = TRUE;

  log.add(4,"info: sending mail from '%s@%s' to '%s@%s'",
            snd->getUser(), snd->getDomain(), rec->getUser(), rec->getDomain());

  // get all MX-Hosts of recipient
  if (dns.getExchanger(rec->getDomain(), &rec->mip[0], &rec->mip[1], &rec->mip[2]) == FALSE)
  {
    log.add(2,"error: cannot resolve domain '%s', send to '%s@%s' failed",
              rec->getDomain(), rec->getUser(), rec->getDomain());
    return(FALSE);
  }

  // if we bind on a specified interface, get ip
  if (sendhosts)
  {
    log.add(5,"info: trying to lookup sendhost: %s", sendhosts);
    sendhost = dns.lookup(sendhosts);
  }

  // connect to all 3 mx-ip's if available and necessary
  while((num < 3) && (rec->mip[num] != 0))
  {
    log.add(5,"info: trying to connect MX (%d/3) [0x%x]", num+1, rec->mip[num]);

    conn = new Connection();
    ASSERT(conn);

    // try to open new tcp-connection
    if (conn->copen(rec->mip[num], remport, sendhost) == FALSE)
    {
      num++;
      log.add(2,"info: failed to connect to: %s (%d/3 MX of '%s')",
                conn->remoteips.c_str(), num, rec->getDomain());
      delete conn;
      conn = NULL;
      continue;
    }

    log.add(4,"info: connected to mail server [%s], waiting for welcome message", conn->remoteips.c_str());

    // smtp - send, read the remote welcome message
    if (readWelcome())
    {
      if (writeHelo())
      {
        if (writeMail())
        {
          if (writeRcpt())
          {
            // only test recipient and exit without sending message
            if (testconnect)
            {
              writeRset();
              successful = TRUE;
            }
            else
            {
              if (writeData() && sendData())
                successful = TRUE;
            }
            writeQuit();
          }
          else
            writeQuit();
        }
        else
          writeQuit();
      }
      else
        writeQuit();
    }
    break;
  }

  // check if message send succeeded
  if (successful)
  {
    if (testconnect)
      log.add(2,"info: testconnect to '%s@%s' successfully finished", rec->getUser(), rec->getDomain());
    else
      log.add(2,"%s=%s@%s [%s], size=%ld, from=%s@%s [%s], proto=%s, id=%s, errs=%ld",
                "sent mail to", rec->getUser(), rec->getDomain(), conn->remoteips.c_str(),
                currMsg->getSize(), snd->getUser(), snd->getDomain(), conn->localips.c_str(),
                "SMTP", currMsg->getName(), errors);
  }
  else
  {
    log.add(3,"info: failed to send mail to '%s@%s', errs=%ld",
              rec->getUser(), rec->getDomain(), errors);

    // cloning and spooling message
    if (!testconnect && toList && !unknownUser)
    {
      ASSERT(ml);
      if (!ml->mlconfig.getBoolValue(KEY_LIST_DISABLE_SPOOL))
      {
        // copy message
        if ((msg = currMsg->clone(listname.c_str())))
        {
          msg->queue("spooled", Q_INIT);
          msg->unlock();
          delete msg;
        }
      }
    }
  }

  // closing tcp-connection (if open)
  if (conn)
  {
    conn->sclose();
    delete conn;
  }
  return(successful);
}

// --------------------------------------- WELCOME ----------------------------------------------
inline BOOL Transmitter::readWelcome(VOID)
{
  ULONG rchars = 0;
  BOOL done = FALSE;

  DEBUGGER("Transmitter::readWelcome()");
  ASSERT(conn);

  // read multiple line welcome
  while (!done)
  {
    // reading welcome message
    rchars = conn->cread((SBYTE *) &buf);
    if ((!rchars) || strncmp(buf, "220", 3))
    {
      log.add(3,"error: got crude welcome message: %s", buf);
      errors ++;
      return(FALSE);
    }

    if ((rchars > 4) && (buf[3] == 0x20))
      done = TRUE;
  }

  return(TRUE);
}

// --------------------------------------- HELO ----------------------------------------------
inline BOOL Transmitter::writeHelo(VOID)
{
  ULONG rchars = 0;
  BOOL done = FALSE;

  DEBUGGER("Transmitter::writeHelo()");
  ASSERT(conn);

  snprintf(buf, MAX_DATA_SIZE, "HELO %s\r\n", config.getValue(KEY_SERVER_DOMAIN));
  conn->cwrite((SBYTE *) &buf);

  while (!done)
  {
    // reading multiple answer to HELO
    rchars = conn->cread((SBYTE *) &buf);
    if ((!rchars) || strncmp(buf, "250", 3))
    {
      log.add(3, NOACCEPT, conn->remoteips.c_str(), "HELO", buf);
      errors ++;
      return(FALSE);
    }

    if ((rchars > 4) && (buf[3] == 0x20))
      done = TRUE;
  }
  return(TRUE);
}

// ----------------------------------------- MAIL ----------------------------------------------
inline BOOL Transmitter::writeMail(VOID)
{
  ULONG rchars = 0;
  Address * adr = NULL;

  DEBUGGER("Transmitter::writeMail()");
  ASSERT(conn);
  ASSERT(currMsg);

  adr = currMsg->getSender();
  ASSERT(adr);

  snprintf(buf, MAX_DATA_SIZE, "MAIL FROM:<%s@%s>\r\n", adr->getUser(), adr->getDomain());
  conn->cwrite((SBYTE *) &buf);

  rchars = conn->cread((SBYTE *) &buf);
  if ((!rchars) || strncmp(buf, "250", 3))
  {
    log.add(3, NOACCEPT, conn->remoteips.c_str(), "MAIL", buf);
    errors ++;
    return(FALSE);
  }
  return(TRUE);
}

// ----------------------------------------- QUIT ----------------------------------------------
inline BOOL Transmitter::writeQuit(VOID)
{
  ULONG rchars = 0;

  DEBUGGER("Transmitter::writeQuit()");
  ASSERT(conn);

  conn->cwrite("QUIT\r\n");
  rchars = conn->cread((SBYTE *) &buf);
  if ((!rchars) || strncmp(buf, "221", 3))
  {
    log.add(3, NOACCEPT, conn->remoteips.c_str(), "QUIT", buf);
    errors ++;
    return(FALSE);
  }
  return(TRUE);
}

// ----------------------------------------- RCPT ----------------------------------------------
inline BOOL Transmitter::writeRcpt(VOID)
{
  ULONG rchars = 0;
  Address * rec = NULL;

  DEBUGGER("Transmitter::writeRcpt()");
  ASSERT(conn);
  ASSERT(currMsg);

  rec = currMsg->getRec();
  ASSERT(rec);

  // sending recipient(s) RCPT TO:
  snprintf(buf, MAX_DATA_SIZE, "RCPT TO:<%s@%s>\r\n", rec->getUser(), rec->getDomain());
  conn->cwrite((SBYTE *) &buf);

  rchars = conn->cread((SBYTE *) &buf);
  if ((!rchars) || strncmp(buf, "250", 3))
  {
    errors ++;
    log.add(3, NOACCEPT, conn->remoteips.c_str(), "RCPT", buf);

    // no such user, bad response, unsubscribing user
    if (!strncmp(buf, "55", 2))
    {
      unknownUser = TRUE;
      if (toList)
      {
        ASSERT(ml);
        if (ml->mlconfig.getBoolValue(KEY_LIST_AUTO_UNSUB) == TRUE)
        {
          log.add(2,"info: user '%s@%s' unknown, unsubscribing", rec->getUser(), rec->getDomain());
          ml->delUser(rec);
        }
        else
          log.add(2,"info: user '%s@%s' unknown, but '%s' not set",
                    rec->getUser(), rec->getDomain(), KEY_LIST_AUTO_UNSUB);
      }
      else
        log.add(2,"info: user '%s@%s' unknown to remote MTA", rec->getUser(), rec->getDomain());
    }
    return(FALSE);
  }
  return(TRUE);
}

// ----------------------------------------- RSET ----------------------------------------------
inline BOOL Transmitter::writeRset(VOID)
{
  ULONG rchars = 0;

  DEBUGGER("Transmitter::writeRset()");
  ASSERT(conn);

  conn->cwrite("RSET\r\n");

  rchars = conn->cread((SBYTE *) &buf);
  if ((!rchars) || strncmp(buf, "250", 3))
  {
    log.add(3, NOACCEPT, conn->remoteips.c_str(), "RSET", buf);
    errors ++;
    return(FALSE);
  }
  return(TRUE);
}

// ----------------------------------------- DATA ----------------------------------------------
inline BOOL Transmitter::writeData(VOID)
{
  DEBUGGER("Transmitter::writeData()");
  ASSERT(conn);

  conn->cwrite("DATA\r\n");
  rchars = conn->cread((SBYTE *) &buf);

  if ((!rchars) || strncmp(buf, "354", 3))
  {
    log.add(3, NOACCEPT, conn->remoteips.c_str(), "DATA", buf);
    errors ++;
    return(FALSE);
  }
  return(TRUE);
}

// -------------------------------------- FILE ----------------------------------------------
VOID Transmitter::sendFile(SBYTE * fname)
{
  Storage * stor = NULL;
  DataBlock block(TYPE_LINE);
  Line * tmp = NULL;
  SBYTE * c = NULL;

  DEBUGGER("Transmitter::sendFile()");
  ASSERT(conn);
  ASSERT(fname);

  stor = new FileStorage(STORE_DATA, MODE_READ);
  ASSERT(stor);

  if (stor->open(fname) == FALSE)
  {
    log.add(2,"error: failed to read header/footer file '%s', ignored", fname);
    return;
  }
  log.add(4,"info: opened messagefile '%s' for reading", fname);

  while (stor->get(&block) == TRUE)
  {
    while (block.get(&tmp) == TRUE)
    {
      // cut return/linefeed
      c = tmp->str;
      while(*c != 0x0d && *c != 0x0a && *c !='\0')
        c++;
      *c = '\0';
      strcat(tmp->str, "\r\n");

      read = strlen(tmp->str);
      wrote = conn->cwrite(tmp->str);
      total += wrote;

      if (wrote < read)
      {
        log.add(3,"error: write on socket failed (wrote %ld/%ld bytes)!!!", wrote, read);
        errors ++;
        stor->close();
        return;
      }
    }
  }

  stor->close();
}

// ----------------------------------------- DATA ----------------------------------------------
BOOL Transmitter::sendData(VOID)
{
  STRING fname;
  Storage * store = NULL;
  DataBlock block(TYPE_LINE);
  Line * lin = NULL;
  Address * rec = NULL;
  SBYTE * bptr = NULL, * spool_dir = config.getValue(KEY_SERVER_SPOOL_DIR);
  SBYTE buffer[MAX_LINE_SIZE+1];

  SBYTE * sreply = NULL, * sfrom = NULL, * sto = NULL, * ssender = NULL, * ssubject = NULL;
  SBYTE * header_file = NULL, * footer_file = NULL;
  BOOL  exreply = FALSE, exfrom = FALSE, exto = FALSE, exsender = FALSE, exsubject = FALSE;
  BOOL  changed = FALSE, header = TRUE, missing = FALSE, didRead, addHeader = FALSE;

  DEBUGGER("Transmitter::sendData()");
  ASSERT(conn);
  ASSERT(currMsg);
  ASSERT(currMsg->mconf);

  rec = currMsg->getRec();
  store = new FileStorage(STORE_DATA, MODE_READ);
  ASSERT(rec);
  ASSERT(store);

  // opening messagefile
  fname = string(spool_dir) + currMsg->getName() + FILE_DATA_EXT;
  if (store->open(fname.c_str()) != TRUE)
  {
    log.add(1,"info: failed to open datafile '%s' for reading, could not send message", fname.c_str());
    errors ++;
    return(FALSE);
  }
  else
    log.add(4,"info: opened datafile '%s' for reading", fname.c_str());

  // prepare change of header
  if (toList)
  {
    ASSERT(ml);
    header_file = ml->mlconfig.getValue(KEY_LIST_HEADER_FILE);
    footer_file = ml->mlconfig.getValue(KEY_LIST_FOOTER_FILE);
    ssubject = ml->mlconfig.getValue(KEY_LIST_HEADER_SUBJECT);
    ssender = ml->mlconfig.getValue(KEY_LIST_HEADER_SENDER);
    sreply = ml->mlconfig.getValue(KEY_LIST_HEADER_REPLY);
    sfrom = ml->mlconfig.getValue(KEY_LIST_HEADER_FROM);
    sto  = ml->mlconfig.getValue(KEY_LIST_HEADER_RECIPIENT);
  }

  // main data sending loop
  while(TRUE)
  {
    changed = FALSE;
    if (!toList || !missing)
    {
      if (store->get(&block) == FALSE || block.get(&lin) == FALSE)
        break;

      ASSERT(lin);
      didRead = TRUE;
      bptr = lin->str;
    }
    else
    {
      snprintf(buffer, MAX_LINE_SIZE, "\n");
      didRead = FALSE;
      bptr = buffer;
    }

    // exchange headers if we broadcast
    if (toList && header)
    {
      read = strlen(bptr);

      if (sreply && (!strncasecmp(bptr, "reply-to:", 9) || (missing && !exreply && !changed)))
      {
        snprintf(bptr, MAX_LINE_SIZE, "Reply-To: %s\r\n", sreply);
        changed = exreply = TRUE;
      }

      if (ssender && (!strncasecmp(bptr, "sender:", 7) || (missing && !exsender && !changed)))
      {
        snprintf(bptr, MAX_LINE_SIZE, "Sender: %s\r\n", ssender);
        changed = exsender = TRUE;
      }

      if (sfrom && (!strncasecmp(bptr, "from:", 5) || (missing && !exfrom && !changed)))
      {
        snprintf(bptr, MAX_LINE_SIZE, "From: %s\r\n", sfrom);
        changed = exfrom = TRUE;
      }

      if (sto && (!strncasecmp(bptr, "to:", 3) || (missing && !exto && !changed)))
      {
        if (!strncasecmp(sto, "exchange", 8))
          snprintf(bptr, MAX_LINE_SIZE, "To: %s@%s\r\n", rec->getUser(), rec->getDomain());
        else
          snprintf(bptr, MAX_LINE_SIZE, "To: %s\r\n", sto);

        changed = exto = TRUE;
      }

      if (ssubject && (!strncasecmp(bptr, "subject:", 8) || (missing && !exsubject && !changed)))
      {
        if (didRead)
        {
          if (!strstr(bptr, ssubject))
          {
            SBYTE * p = NULL;
            if ((p =  new SBYTE[MAX_LINE_SIZE+1]) != NULL)
            {
              strcpy(p, bptr+8);
              snprintf(bptr, MAX_LINE_SIZE, "Subject: %s%s", ssubject, p);
              delete p;
            }
            else
              log.add(1,"error: malloc failed, mem allocation error!!!");
          }
        }
        else
          snprintf(bptr, MAX_LINE_SIZE, "Subject: %s\r\n", ssubject);

        changed = exsubject = TRUE;
      }

      if (changed)
        log.add(4,"info: header has been added/changed -> %s", bptr);

      if ((header && (read == 2)) || (missing && !changed))
      {
        if ((!sreply == !exreply) && (!sto == !exto) && (!sfrom == !exfrom)
          && (!ssender == !exsender) && (!ssubject == !exsubject))
        {
          snprintf(bptr, MAX_LINE_SIZE, "\r\n");
          missing = FALSE;
          header = FALSE;
          addHeader = TRUE;
        }
        else
        {
          missing = TRUE;
          continue;
        }
      }
    }

    read = strlen(bptr);

    // do not send end of mail here
    if ((!header) && (read == 3) && bptr[0] == '.')
    {
      log.add(4,"warning: got end of mail sequence, stop sending");
      errors ++;
      break;
    }

    // finally write data to socket
    wrote = conn->cwrite(bptr);
    total += wrote;

    if (wrote < read)
    {
      log.add(3,"error: write on socket failed (wrote %ld/%ld bytes)!!!", wrote, read);
      errors ++;
      break;
    }

    if (didRead)
      delete lin;

    // sending message header file if requested
    if (toList && header_file && addHeader)
    {
      addHeader = FALSE;
      sendFile(header_file);
    }
  }

  // closing messagefile
  store->close();
  delete store;

  // sending message footer file if requested
  if (toList && footer_file)
    sendFile(footer_file);

  // sending end of mail <CRLF>.<CRLF>
  snprintf(buffer, MAX_LINE_SIZE, ".\r\n");
  read = strlen(buffer);
  wrote = conn->cwrite((SBYTE *) &buffer);
  total += wrote;

  if (wrote < read)
  {
    log.add(3,"error: write on socket failed (wrote %ld/%ld bytes)!!!", wrote, read);
    return(FALSE);
    errors ++;
  }

  // reading 250 ok ...
  if ((!conn->cread((SBYTE *) &buf)) || strncmp(buf, "250", 3))
  {
    log.add(3, NOACCEPT, conn->remoteips.c_str(), "<CRLF>.<CRLF>", buf);
    errors ++;
    return(FALSE);
  }

  log.add(3,"info: sent mail to '%s@%s', %ld bytes #> %s",
            rec->getUser(), rec->getDomain(), total, buf);
  return(TRUE);
}

// denying connection
BOOL Transmitter::deny(VOID)
{
  DEBUGGER("Transmitter::deny()");
  return(FALSE);
}

// global signal handler (transmitter)
static VOID trans_signal(INT sig)
{
  switch(sig)
  {
    case(SIGPIPE):
      log.add(1,"info: transmitter received SIGPIPE (socket closed by remote), exiting");
      exit(FALSE);
      break;

    case(SIGINT):
      log.add(1,"error: stop signal received by transmitter, exiting");
      exit(FALSE);
      break;

    default:
      log.add(1,"error: transmitter got unknown signal %d, ignored", sig);
      break;
  }
  return;
}
