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

#include "receiver_def.h"
#include "datablock_inl.h"
#include "connection_inl.h"

#include "scheduler_def.h"
#include "scheduler_inl.h"

extern Log  log;
extern Timer  timer;
extern Config  config;
extern Scheduler  scheduler;
extern Resolver  dns;
extern List<MailingList>  lists;
extern Task * task;

static VOID rec_signal(INT sig);
extern MailingList * getList(const SBYTE * listname, const SBYTE * listdomain = NULL);

SBYTE * NOACCEPT = "error: remote MTA (%s) did not accept '%s', sent: %s";

Receiver::Receiver(VOID)
{
  errors = 0;
  status = CON_NONE;
  currMsg = NULL;
  currAdd = NULL;
  conn = NULL;
  authorized = FALSE;
}

Receiver::Receiver(Connection * conn)
{
  errors = 0;
  status = CON_NONE;
  currMsg = NULL;
  currAdd = NULL;

  ASSERT(conn);
  this->conn = conn;
}

Receiver::~Receiver()
{
}

VOID Receiver::setConn(Connection * conn)
{
  DEBUGGER("Receiver::setConn()");
  ASSERT(conn);

  this->conn = conn; 
}

BOOL Receiver::start(VOID)
{
  SMTP_CMD cmd = NOOP;
  BOOL quit = FALSE;
  BOOL ok = TRUE;
  INT x;

  DEBUGGER("Receiver::start()");

  // installing signal handlers
  sigINT  = new SignalHandler(SIGINT, rec_signal);
  sigHUP  = new SignalHandler(SIGHUP, rec_signal);
  sigPIPE = new SignalHandler(SIGPIPE, rec_signal);
  sigCHLD = new SignalHandler(SIGCHLD, rec_signal);
  sigALRM = new SignalHandler(SIGALRM, rec_signal);

  ASSERT(conn);

  authorized = FALSE;
  errors = 0;

  log.add(4,"info: receiving from '%s' [%s]", conn->remotehost.c_str(), conn->remoteips.c_str());

  // creating message
  currMsg = new Message(); 
  ASSERT(currMsg);
  currMsg->init();

  // sending welcome message to remote MTA
  writeWelcome();

  // major smtp communication loop
  while (TRUE)
  {
    rchars = conn->cread(buf);

    // tolower conversion of the first 4 chars
    for (x = 0; x<4; x++)
      if ((buf[x] > 0x40) && (buf[x] < 0x5B))
        buf[x] += 0x20;

    // did we get a correct SMTP-Command ? 
    if (!((rchars == 6) || ((buf[4] == 0x20) && (rchars > 6))))
    {
      conn->cwrite(SMTP_500_UNRE);
      errors ++;
    }
    else
    {
      ok = TRUE;
      cmd = (enum SMTP_CMD) (buf[0]+(buf[1]*10)+(buf[2]*100)+(buf[3]*1000));
      switch(cmd)
      {
        case(EXPN):
        case(SAML):
        case(SEND):
        case(SOML):
        case(TURN):
        case(EHLO): conn->cwrite(SMTP_502_NOOP);
                    ok = FALSE;
                    break;
        case(HELO): ok = readHelo();
                    break;
        case(LADD): ok = readLadd();
                    break;
        case(LDEL): ok = readLdel();
                    break;
        case(LPWD): ok = readLpwd();
                    break;
        case(LRLD): ok = readLrld();
                    break;
        case(LCMD): ok = readLcmd();
                    break;
        case(LSHW): ok = readLshw();
                    break;
        case(RSET): ok = readRset();
                    break;
        case(MAIL): ok = readMail();
                    break;
        case(RCPT): ok = readRcpt();
                    break;
        case(HELP): conn->cwrite(SMTP_214_HELP);
                    break;
        case(NOOP): conn->cwrite(SMTP_250_OKAY);
                    break;
        case(VRFY): conn->cwrite(SMTP_252_VRFY);
                    break;
        case(DATA): if (readData() == FALSE)
                    {
                      ok = FALSE;
                      conn->cwrite(SMTP_552_ABRT);
                    }
                    break;
        case(QUIT): quit = readQuit();
                    break;
        default:    conn->cwrite(SMTP_500_UNRE);
                    ok = FALSE;
                    log.add(3,"info: got unknown SMTP command: '%s' (%d)", buf, cmd);
                    break;
      }
    }

    if (quit == TRUE)
      break;

    if (ok == FALSE)
      errors ++;

    if (conn->getErrors() + errors >= MAX_SMTP_ERRORS)
    {
      log.add(3,"error: too many errors (%ld) occured during receive from: %s",
              conn->getErrors() + errors, conn->remoteips.c_str());
      break;
    }
  }
  conn->cclose(TRUE);

  // if receive succeeded, put message in queue
  if (status == CON_DONE)
  {
    // queue and remove lock-file of message
    currMsg->queue("received", Q_INIT);
    currMsg->unlock();
  }
  delete currMsg; 
  return(status == CON_DONE?TRUE:FALSE);
}

// -------------------------------------- WELCOME ----------------------------------------------
inline BOOL Receiver::writeWelcome(VOID)
{
  SBYTE * fake = config.getValue(KEY_SERVER_FAKE_NAME);

  DEBUGGER("Receiver::writeWelcome()");
  ASSERT(conn);

  snprintf(buf, MAX_DATA_SIZE, "220 %s SMTP %s; %s\n",
             conn->localhost.c_str(), fake?fake:LIMEZSTRING, timer.getSMTPDate());

  conn->cwrite((SBYTE *) &buf);
  return(TRUE);
}

// ----------------------------------------- QUIT ----------------------------------------------
inline BOOL Receiver::readQuit(VOID)
{
  DEBUGGER("Receiver::readQuit()");
  ASSERT(conn);

  snprintf(buf, MAX_DATA_SIZE, "221 %s closing connection\r\n", conn->localhost.c_str());
  conn->cwrite((SBYTE *) &buf); 

  if (status == CON_DATA)
    status = CON_DONE;

  return(TRUE);
}

// ----------------------------------------- LPWD ----------------------------------------------
inline BOOL Receiver::readLpwd(VOID)
{
  const SBYTE * host = NULL;
  SBYTE * pass = config.getValue(KEY_SERVER_WEB_PASS);
  SBYTE * ip = NULL;
  UWORD num = 0;

  DEBUGGER("Receiver::readLpwd()");
  ASSERT(conn);
  ASSERT(pass);

  host = conn->remoteips.c_str();

  if (status > CON_NONE || authorized == TRUE)
  {
    conn->cwrite(SMTP_501_SYNT);
    return(FALSE);
  }
  else
  {
    buf[strlen(buf)-2]='\0';

    if (pass)
    {
      if (!strcmp(pass, buf+5))
      {
        log.add(4,"info: given password '%s' matches", buf+5);

        // check remote ip
        while ((ip = config.getNext(++num, KEY_SERVER_WEB_HOSTS)))
        {
          if (!strncmp(ip, host, strlen(ip)))
          {
            log.add(4,"info: host '%s' matches '%s'", host, ip);
            authorized = TRUE;
          }
        }
      }
    }
  }

  if (authorized == TRUE)
  {
    conn->cwrite("250 password accepted\r\n");
    return(TRUE);
  }
  else
  {
    sleep(5);
    conn->cwrite("501 password/ip mismatch or not set\r\n");
    log.add(1,"warning: authorization (ip: %s) failed, got pwd '%s'", host, buf+5);
    return(FALSE);
  }
}

// ----------------------------------------- LRLD ----------------------------------------------
inline BOOL Receiver::readLrld(VOID)
{
  DEBUGGER("Receiver::readLrld()");
  ASSERT(conn);

  if (status > CON_NONE)
  {
    conn->cwrite(SMTP_501_SYNT);
    return(FALSE);
  }
  else
  {
    if (authorized == TRUE)
    {
      if (!kill(getppid(), SIGHUP))
        conn->cwrite("250 reload succeeded\r\n");
      else
      {
        conn->cwrite("501 reload failed\r\n");
        return(FALSE);
      }
    }
    else
    {
      conn->cwrite(SMTP_501_NOAU);
      return(FALSE);
    }
  }
  return(TRUE);
}

// ----------------------------------------- LDEL ----------------------------------------------
inline BOOL Receiver::readLdel(VOID)
{
  Storage * fs = NULL;
  SBYTE * fn = NULL;
  MailingList * ml;
  BOOL found = FALSE;
  SBYTE * key = NULL, * val = NULL, * list = NULL, * x = NULL, * cfg = NULL;

  DEBUGGER("Receiver::readLdel()");
  ASSERT(conn);

  if (status > CON_NONE)
  {
    conn->cwrite(SMTP_501_SYNT);
    return(FALSE);
  }
  else
  {
    if (authorized == TRUE)
    {
      // delete list
      if (!strncasecmp("list ", buf+5, 5))
      {
        found = TRUE;
        list = x = buf+10;
        while (*x != 0x20 && *x != 0x0d && *x != 0x0a && *x != '\0') x++;
          *x = '\0';

        // check all listnames
        if ((ml = getList(list)))
        {
          cfg = ml->mlconfig.getValue(KEY_LIST_CONFIG);
          ASSERT(cfg);

          if (!strcmp(cfg, USE_DB_STORAGE))
            config.delValue(KEY_MAILINGLIST_DB, list);
          else
            config.delValue(KEY_MAILINGLIST, cfg);

          config.write(config.getValue(KEY_SERVER_CONFIG));
          ml->remove();
          lists.del(ml);
          delete ml;
          conn->cwrite(SMTP_250_OKAY);
        }
        else
          conn->cwrite("501 unknown mailing list\r\n");
      }

      // delete config entry
      if (!strncasecmp("config ", buf+5, 7))
      {
        found = TRUE;

        // get listname
        list = x = buf+12;
        while (*x != 0x20 && *x != 0x0d && *x != 0x0a && *x != '\0') x++;
          *x = '\0';

        // get key
        key = ++x;
        while (*x != 0x20 && *x != 0x0d && *x != 0x0a && *x != '\0') x ++;
          *x = '\0';

        // get value
        val = ++x;
        while (*x != 0x0d && *x != 0x0a && *x != '\0') x ++;
          *x = '\0';

        // add to limez main config
        if (!strcasecmp("limez", list))
        {
          if (config.delValue(key, val) == TRUE)
          {
            // add config entry (limez/list)
            if (config.write(config.getValue(KEY_SERVER_CONFIG)) == TRUE)
              conn->cwrite("250 ok, server needs to reload configs\r\n");
            else
              conn->cwrite("553 error occured while saving config\r\n");
          }
          else
              conn->cwrite("553 error occured while removing entry\r\n");
        }
        else
        {
          // add to config of given list
          if ((ml = getList(list)))
          {
            found = TRUE;
            if (ml->mlconfig.delValue(key, val) == TRUE)
            {
              // add config entry list
              if (ml->mlconfig.write(ml->mlconfig.getValue(KEY_LIST_CONFIG)) == TRUE)
                conn->cwrite("250 ok, server needs to reload\r\n");
              else
                conn->cwrite("553 error occured while saving config\r\n");
            }
            else
                conn->cwrite("553 error occured while removing entry\r\n");
          }
          else
            conn->cwrite("501 unknown mailing list\r\n");
        }
      }

      // delete requested file
      if (!strncasecmp("file ", buf+5, 5))
      {
        found = TRUE;
        fn = (SBYTE *) (buf + 10);
        buf[strlen(buf)-2] = '\0';

        if ((*fn != '/') && (strstr(fn, "..") == NULL))
        {
          fs = new FileStorage(STORE_DATA, MODE_DELETE);
          ASSERT(fs);
          if (fs->open(fn) == TRUE)
          {
            fs->destroy(fn);
            fs->close();
            conn->cwrite(SMTP_250_OKAY);
            log.add(3,"info: storage '%s' deleted with LDEL", fn);
          }
          else
          {
            log.add(1,"error: cannot open storage '%s'", fn);
            conn->cwrite("553 error occured\r\n");
          }
          delete fs;
        }
        else
        {
          log.add(1,"warning: someone (IP %s) tried to access '%s' via LADD",
                    conn->remoteips.c_str(), fn);
          conn->cwrite("553 access denied, administrator has been informed\r\n");
        }
      }

      if (found == FALSE)
      {
        conn->cwrite("501 unknown limez delete (LDEL) sub-command\r\n");
        return(FALSE);
      }
    }
    else
    {
      conn->cwrite(SMTP_501_NOAU);
      return(FALSE);
    }
    return(TRUE);
  }
}

// ----------------------------------------- LADD ----------------------------------------------
inline BOOL Receiver::readLadd(VOID)
{
  Storage * fs = NULL;
  SBYTE * fn = NULL;
  Line * ln = NULL;
  BOOL found = FALSE;
  DataBlock  block(TYPE_LINE);
  SBYTE * key = NULL, * val = NULL, * list = NULL, * x = NULL, * nam = NULL;
  MailingList * ml = NULL;
  Address * adr = NULL;

  DEBUGGER("Receiver::readLadd()");
  ASSERT(conn);

  if (status > CON_NONE)
  {
    conn->cwrite(SMTP_501_SYNT);
    return(FALSE);
  }
  else
  {
    if (authorized == TRUE)
    {
      // add new db list to config
      if (!strncasecmp("dblist ", buf+5, 7))
      {
        found = TRUE;
        list = x = buf+12;
        while (*x != 0x20 && *x != 0x0d && *x != 0x0a && *x != '\0') x++;
          *x = '\0';

        adr = new Address();
        nam = adr->setUserName(list);

        if (!nam)
        {
          if (adr->isValidRec(FALSE) == REC_NONE)
          {
            if (config.addValue(KEY_MAILINGLIST_DB, adr->getUser()) == TRUE)
            {
              if (config.write(config.getValue(KEY_SERVER_CONFIG)) == TRUE)
                conn->cwrite("250 ok, server needs to reload configs\r\n");
              else
                conn->cwrite("553 error occured while saving config\r\n");
            }
            else
                conn->cwrite("553 error occured while adding value\r\n");
          }
          else
            conn->cwrite("501 error, wrong name given or already in use\r\n");
        }
        else
        {
          snprintf(buf, MAX_DATA_SIZE, "501 error: %s\r\n", nam);
          conn->cwrite((SBYTE *) &buf);
        }
        delete adr;
      }

      // add new file list to config
      if (!strncasecmp("list ", buf+5, 5))
      {
        found = TRUE;
        list = x = buf+10;
        while (*x != 0x20 && *x != 0x0d && *x != 0x0a && *x != '\0') x++;
          *x = '\0';

        SBYTE * fnam = x+1;
        while (*x != 0x20 && *x != 0x0d && *x != 0x0a && *x != '\0') x++;
          *x = '\0';

        // crop CR
        fnam[strlen(fnam)-2] = '\0';

        adr = new Address();
        nam = adr->setUserName(list);

        if (!nam && fnam)
        {
          if (adr->isValidRec(FALSE) == REC_NONE)
          {
            if ((*fnam != '/') && (strstr(fnam, "..") == NULL))
            {
              // writing config file
              fs = new FileStorage(STORE_DATA, MODE_WRITE);
              ASSERT(fs);
              if (fs->open(fnam) == TRUE)
              {
                ln = new Line;
                snprintf(ln->str, MAX_LINE_SIZE, "list_name %s\n", adr->getUser());
                block.add(ln);

                ln = new Line;
                snprintf(ln->str, MAX_LINE_SIZE, "list_config %s\n", fnam);
                block.add(ln);

                fs->add(&block);
                fs->close();

                // adding key in main config
                if (config.addValue(KEY_MAILINGLIST, fnam) == TRUE)
                {
                  if (config.write(config.getValue(KEY_SERVER_CONFIG)) == TRUE)
                    conn->cwrite("250 ok, server needs to reload configs\r\n");
                  else
                    conn->cwrite("553 error occured while saving config\r\n");
                }
                else
                    conn->cwrite("553 error occured while adding value\r\n");
              }
              else
              {
                log.add(1,"error: cannot open/create storage '%s'", fnam);
                conn->cwrite("553 error occured while creating config file\r\n");
              }
              delete fs;
            }
            else
              conn->cwrite("501 error, invalid filename\r\n");
          }
          else
            conn->cwrite("501 error, wrong name given or already in use\r\n");
        }
        else
        {
          snprintf(buf, MAX_DATA_SIZE, "501 error: %s/invalid filename\r\n", nam);
          conn->cwrite((SBYTE *) &buf);
        }
        delete adr;
      }

      // add new entry in limez configuration
      if (!strncasecmp("config ", buf+5, 7))
      {
        found = TRUE;

        // get listname
        list = x = buf+12;
        while (*x != 0x20 && *x != 0x0d && *x != 0x0a && *x != '\0') x++;
          *x = '\0';

        // get key
        key = ++x;
        while (*x != 0x20 && *x != 0x0d && *x != 0x0a && *x != '\0') x ++;
          *x = '\0';

        // get value
        val = ++x;
        while (*x != 0x0d && *x != 0x0a && *x != '\0') x ++;
          *x = '\0';

        // add to limez main config
        if (!strcasecmp("limez", list))
        {
          if (config.addValue(key, val) == TRUE)
          {
            // add config entry (limez/list)
            if (config.write(config.getValue(KEY_SERVER_CONFIG)) == TRUE)
              conn->cwrite("250 ok, server needs to reload configs\r\n");
            else
              conn->cwrite("553 error occured while saving entry\r\n");
          }
          else
              conn->cwrite("553 error occured while adding entry\r\n");
        }
        else
        {
          // add to config of given list
          if ((ml = getList(list)))
          {
            found = TRUE;
            if (ml->mlconfig.addValue(key, val) == TRUE)
            {
              // add config entry list
              if (ml->mlconfig.write(ml->mlconfig.getValue(KEY_LIST_CONFIG)) == TRUE)
                conn->cwrite("250 ok, server needs to reload\r\n");
              else
                conn->cwrite("553 error occured while saving entry\r\n");
            }
            else
                conn->cwrite("553 error occured while adding entry\r\n");
          }
          else
            conn->cwrite("501 unknown mailing list\r\n");
        }
      }

      // create/overwrite requested file
      if (!strncasecmp("file ", buf+5, 5))
      {
        found = TRUE;
        fn = (SBYTE *) (buf + 10);
        buf[strlen(buf)-2] = '\0';

        if ((*fn != '/') && (strstr(fn, "..") == NULL))
        {
          fs = new FileStorage(STORE_DATA, MODE_WRITE);
          ASSERT(fs);
          if (fs->open(fn) == TRUE)
          {
            conn->cwrite(SMTP_250_OKAY);
            while(TRUE)
            {
              rchars = conn->cread(buf);
              if (!rchars || strncmp(buf, "250-", 4))
                break;

              block.add(new Line(buf+4));
              fs->add(&block);
            }

            fs->close();
            conn->cwrite(SMTP_250_OKAY);
          }
          else
          {
            log.add(1,"error: cannot open/create storage '%s'", fn);
            conn->cwrite("553 error occured\r\n");
          }
          delete fs;
        }
        else
        {
          log.add(1,"warning: someone (IP %s) tried to access '%s' via LADD",
                    conn->remoteips.c_str(), fn);
          conn->cwrite("553 access denied, administrator has been informed\r\n");
        }
      }

      if (found == FALSE)
      {
        conn->cwrite("501 unknown limez add (LADD) sub-command\r\n");
        return(FALSE);
      }
    }
    else
    {
      conn->cwrite(SMTP_501_NOAU);
      return(FALSE);
    }
    return(TRUE);
  }
}

// ----------------------------------------- LCMD ----------------------------------------------
inline BOOL Receiver::readLcmd(VOID)
{
  DEBUGGER("Receiver::readLcmd()");
  ASSERT(conn);
  Message * tmp = NULL;
  Line * ln = NULL;
  Address * adr = NULL;

  if (status > CON_NONE)
  {
    conn->cwrite(SMTP_501_SYNT);
    return(FALSE);
  }
  else
  {
    if (authorized == TRUE)
    {
      adr = new Address;
      ASSERT(adr);
      adr->setUserName(config.getValue(KEY_SERVER_MAIL));
      adr->setDomainName(config.getValue(KEY_SERVER_DOMAIN));

      tmp = new Message();
      ASSERT(tmp);
      tmp->setSender(adr);
      tmp->addRec(adr);
      tmp->parseCommand(TRUE, FALSE, buf+5);

      for (ln = tmp->lines.getFirst(); ln; ln = tmp->lines.getNext(ln))
      {
        snprintf(buf, MAX_DATA_SIZE, "250-%s", ln->str);
        conn->cwrite((SBYTE *) &buf);
      }
      delete tmp;
      conn->cwrite("250 cmd accepted\r\n");
    }
    else
    {
      conn->cwrite(SMTP_501_NOAU);
      return(FALSE);
    }
  }
  return(TRUE);
}

// ----------------------------------------- LSHW ----------------------------------------------
inline BOOL Receiver::readLshw(VOID)
{
  MailingList * ml = NULL;
  Config * tc = NULL;
  Address * adr = NULL;
  Storage * fs = NULL;
  BOOL found = FALSE;
  DataBlock  block(TYPE_LINE);
  DataBlock  ablock(TYPE_ADDRESS);
  SBYTE * fn = NULL, * nm = NULL;
  Line * ln = NULL;
  ULONG num = 0;

  DEBUGGER("Receiver::readLshw()");
  ASSERT(conn);

  if (status > CON_NONE)
  {
    conn->cwrite(SMTP_501_SYNT);
    return(FALSE);
  }
  else
  {
    if (authorized == TRUE)
    {
      buf[strlen(buf)-2]='\0';

      // show contents of configuration files
      if (!strncasecmp("config ", buf+5, 7))
      {
        // limez main config
        if (!strcasecmp("limez", buf+12))
        {
          // show main config
          config.dump(conn);
          found = TRUE;
          conn->cwrite(SMTP_250_OKAY);
        }
        else
        {
          // show config of given list
          if ((ml = getList(buf+12)))
          {
            found = TRUE;
            // dump config
            ml->mlconfig.dump(conn);
            conn->cwrite(SMTP_250_OKAY);
          }
          else
            conn->cwrite("501 unknown mailing list\r\n");
        }
      }

      // return the names of all configured lists
      if (!strcasecmp("lists", buf+5))
      {
        found = TRUE;
        ml = lists.getFirst();
        while (ml)
        {
          snprintf(buf, MAX_DATA_SIZE, "250-%s\r\n", ml->mlconfig.getValue(KEY_LIST_NAME));
          conn->cwrite((SBYTE *) &buf);
          ml = lists.getNext(ml);
        }
        conn->cwrite(SMTP_250_OKAY);
      }

      // return all possible keys, default values and options for mainconfig
      if (!strcasecmp("mainkeys", buf+5))
      {
        // show main config keys
        config.dumpKeys(conn);
        found = TRUE;
        conn->cwrite(SMTP_250_OKAY);
      }

      // return all possible keys, default values and options for listconfig
      if (!strcasecmp("listkeys", buf+5))
      {
        // show list config keys (we had to hack a little bit)
        tc = new Config("tmp");
        ASSERT(tc);
        tc->initListKeys();
        tc->dumpKeys(conn);
        delete tc;
        found = TRUE;
        conn->cwrite(SMTP_250_OKAY);
      }

      // return server info
      if (!strcasecmp("info", buf+5))
      {
        snprintf(buf, MAX_DATA_SIZE, "250-\"VERSION\" \"%s\"\r\n", VERSION);
        conn->cwrite((SBYTE *) &buf);
        snprintf(buf, MAX_DATA_SIZE, "250-\"UPTIME\" \"%s\"\r\n", timer.uptimeString());
        conn->cwrite((SBYTE *) &buf);
        snprintf(buf, MAX_DATA_SIZE, "250-\"SERVER_TIME\" \"%ld\"\r\n", timer.getUnix());
        conn->cwrite((SBYTE *) &buf);
        snprintf(buf, MAX_DATA_SIZE, "250-\"RELOADS\" \"%ld\"\r\n", scheduler.getReloads());
        conn->cwrite((SBYTE *) &buf);
        snprintf(buf, MAX_DATA_SIZE, "250-\"RUNNING_TASKS\" \"%ld\"\r\n", scheduler.getTasks());
        conn->cwrite((SBYTE *) &buf);
        snprintf(buf, MAX_DATA_SIZE, "250-\"CONNECTIONS\" \"%ld\"\r\n", scheduler.getConns());
        conn->cwrite((SBYTE *) &buf);
        snprintf(buf, MAX_DATA_SIZE, "250-\"CONNECTION_PID\" \"%ld\"\r\n", task->getPid());
        conn->cwrite((SBYTE *) &buf);
        snprintf(buf, MAX_DATA_SIZE, "250-\"CLIENT_IP\" \"%s\"\r\n", conn->remoteips.c_str());
        conn->cwrite((SBYTE *) &buf);
        snprintf(buf, MAX_DATA_SIZE, "250-\"SERVER_IP\" \"%s\"\r\n", conn->localips.c_str());
        conn->cwrite((SBYTE *) &buf);
        snprintf(buf, MAX_DATA_SIZE, "250-\"DOMAIN\" \"%s\"\r\n", config.getValue(KEY_SERVER_DOMAIN));
        conn->cwrite((SBYTE *) &buf);

        conn->cwrite(SMTP_250_OKAY);
        found = TRUE;
      }

      // show requested file
      if (!strncasecmp("file ", buf+5, 5))
      {
        fn = (SBYTE *) (buf + 10);
        if ((*fn != '/') && (strstr(fn, "..") == NULL))
        {
          fs = new FileStorage(STORE_DATA, MODE_READ);
          ASSERT(fs);
          if (fs->open(fn) == TRUE)
          {
            while (fs->get(&block) == TRUE)
            {
              if (block.get(&ln) == TRUE)
              {
                snprintf(buf, MAX_DATA_SIZE, "250-%s", ln->str);
                conn->cwrite((SBYTE *) &buf);
              }
              else
              {
                log.add(1,"error: cannot read from storage '%s'", fn);
                conn->cwrite("553 error occured\r\n");
              } 
            }

            fs->close();
            conn->cwrite(SMTP_250_OKAY);
          }
          else
          {
            log.add(1,"error: cannot open storage '%s'", fn);
            conn->cwrite("553 error occured\r\n");
          }

          delete fs;
        }
        else
        {
          log.add(1,"warning: someone (IP %s) tried to access '%s' via LSHW",
                    conn->remoteips.c_str(), fn);
          conn->cwrite("553 access denied, administrator has been informed\r\n");
        }
        found = TRUE;
      }

      // show all users of given list
      if (!strncmp("usercount ", buf+5, 10))
      {
        found = TRUE;
        if ((ml = getList(buf+15)))
        {
          snprintf(buf, MAX_DATA_SIZE, "250 %ld\r\n", ml->getUserCount());
          conn->cwrite((SBYTE *) &buf);
        }
        else
          conn->cwrite("501 unknown mailing list\r\n");
      }

      // show all users of given list
      if (!strncmp("user ", buf+5, 5))
      {
        nm = buf+10;
        while (*nm != 0x20 && *nm != 0x0a && *nm != 0x0d)
          nm++;

        // get number
        *nm = '\0';
        if (++nm)
          num = atol(nm);

        found = TRUE;
        if ((ml = getList(buf+10)))
        {
          if (ml->getAllUsers(&ablock, num, SHOW_USER_ONCE) == TRUE)
          {
            while (ablock.get(&adr) == TRUE)
            {
              snprintf(buf, MAX_DATA_SIZE, "250-%s@%s\r\n", adr->getUser(), adr->getDomain());
              conn->cwrite((SBYTE *) &buf);
            }
          }
          conn->cwrite(SMTP_250_OKAY);
        }
        else
          conn->cwrite("501 unknown mailing list\r\n");
      }

      // show sender of given list
      if (!strncasecmp("sender ", buf+5, 7))
      {
        // check all listnames and return config of given list
        found = TRUE;
        if ((ml = getList(buf+12)))
        {
          // read sender of list
          ml->getAllSender(&ablock);

          while (ablock.get(&adr) == TRUE)
          {
            snprintf(buf, MAX_DATA_SIZE, "250-%s@%s\r\n", adr->getUser(), adr->getDomain());
            conn->cwrite((SBYTE *) &buf);
          }
          conn->cwrite(SMTP_250_OKAY);
        }
        else
          conn->cwrite("501 unknown mailing list\r\n");
      }

      // show contents of given directory (needed for spool and done) TODO

      if (found == FALSE)
      {
        conn->cwrite("501 unknown limez show (LSHW) sub-command\r\n");
        return(FALSE);
      }
    }
    else
    {
      conn->cwrite(SMTP_501_NOAU);
      return(FALSE);
    }
  }
  return(TRUE);
}

// ----------------------------------------- RSET ----------------------------------------------
inline BOOL Receiver::readRset(VOID)
{
  DEBUGGER("Receiver::readRset()");
  ASSERT(conn);

  status = CON_HELO;

  // reinitialize message, delete sender and recipients
  currMsg->init();

  conn->cwrite(SMTP_250_RSET);
  return(TRUE);
}

// ----------------------------------------- HELO ----------------------------------------------
inline BOOL Receiver::readHelo(VOID)
{
  DEBUGGER("Receiver::readHelo()");
  ASSERT(conn);

  if ((buf[4] != 0x20) && (rchars > 6)); 
  else
  { 
    if ((buf[4] == 0x20) && (rchars > 6)) 
    {
      if (status > CON_NONE)
      {
        snprintf(buf, MAX_DATA_SIZE, "503 %s Duplicate HELO/EHLO\r\n", conn->localhost.c_str());
        conn->cwrite((SBYTE *) &buf);
        return(FALSE);
      }
      else
      {   
        snprintf(buf, MAX_DATA_SIZE, "250 %s Hello %s [%s], pleased to meet you\r\n",
          conn->localhost.c_str(), conn->remotehost.c_str(), conn->remoteips.c_str());

        ASSERT(currMsg);
        ASSERT(currMsg->mconf);
        currMsg->mconf->addValue(KEY_MESG_SEND, conn->remoteips.c_str());

        conn->cwrite((SBYTE *) &buf); 
        status = CON_HELO;
      }
    }
    else
    {
      conn->cwrite(SMTP_501_HELO);
      return(FALSE);
    }
  } 
  return(TRUE);
}

// -------------------------------------- MAIL FROM: -------------------------------------------
inline BOOL Receiver::readMail(VOID)
{
  SBYTE * res = NULL;

  DEBUGGER("Receiver::readMail()");
  ASSERT(conn);

  if (status < CON_HELO)
    conn->cwrite(SMTP_503_SEQ1);
  else
  {
    if (!strncasecmp(buf,"mail from:",10))
    {
      if (rchars > 12)
      {
        if (!(status > CON_HELO))
        {
          buf[rchars-2] = '\0';
         
          currAdd = new Address;
          ASSERT(currAdd);

          if ((res = currAdd->setIfValid((SBYTE *) buf+10)) != NULL)
          {
            snprintf(buf, MAX_DATA_SIZE, "553 %s... %s\r\n", (SBYTE *) buf+10, res);
            conn->cwrite((SBYTE *) &buf);
            delete currAdd;
          }
           else
          {
            snprintf(buf, MAX_DATA_SIZE, "250 %s@%s... Sender ok\r\n",
                     currAdd->getUser(), currAdd->getDomain());

            conn->cwrite((SBYTE *) &buf);
            currMsg->setSender(currAdd);
            status = CON_FROM;
          }          
        }
        else
           conn->cwrite(SMTP_503_SEQ5);
      }
      else
        conn->cwrite(SMTP_501_SYNT);
    }
    else
      conn->cwrite(SMTP_501_SYNT);
  }
  return(TRUE);
}

// --------------------------------------- RCPT TO: --------------------------------------------
inline BOOL Receiver::readRcpt(VOID)
{
  SBYTE * res = NULL;

  DEBUGGER("Receiver::readRcpt()");
  ASSERT(conn);

  if (status < CON_FROM)
     conn->cwrite(SMTP_503_SEQ2);
  else
    if (!strncasecmp(buf,"rcpt to:",8))
    {
      if (rchars > 10)
      {
        buf[rchars-2] = '\0';

        currAdd = new Address;
        ASSERT(currAdd);

        if ((res = currAdd->setIfValid((SBYTE *) buf+8)) != NULL)
        {
          snprintf(buf, MAX_DATA_SIZE, "553 %s... %s\r\n", (SBYTE *) buf+8, res);
          delete currAdd;
          conn->cwrite((SBYTE *) &buf);
        }
        else
        {
          if (currAdd->isValidRec() != REC_NONE)
          {
            snprintf(buf, MAX_DATA_SIZE, "250 %s@%s... Recipient ok\r\n",
                          currAdd->getUser(), currAdd->getDomain());
            conn->cwrite((SBYTE *) &buf);
            currMsg->addRec(currAdd);
            status = CON_RCPT;
          }
          else
          {
            delete currAdd;
            conn->cwrite(SMTP_550_ERRO);
          }
        }
      }
      else
        conn->cwrite(SMTP_501_SYNT);
    }
    else
      conn->cwrite(SMTP_501_SYNT);

  return(TRUE);
}

// -------------------------------- DATA --------------------------------------
BOOL Receiver::readData()
{
  RawData * raw = NULL;
  Storage * store = NULL;
  DataBlock block(TYPE_RAW);

  STRING  mesgname, filename;
  BOOL    pretending = FALSE, clearline = FALSE;
  ULONG   totalsize = 0, received = 0;
  SBYTE * spool_dir = config.getValue(KEY_SERVER_SPOOL_DIR);
  SBYTE * fake = config.getValue(KEY_SERVER_FAKE_NAME);
  ssize_t len;

  DEBUGGER("Receiver::readData()");
  ASSERT(conn);

  if (status < CON_RCPT)
  {
    conn->cwrite(SMTP_503_SEQ4);
    return(TRUE);
  }
  else
    conn->cwrite(SMTP_354_DATA);

  // TODO checking if spool-dir exists

  // preparing data-file
  mesgname = string(timer.getRDate()) + "-" + spid;
  currMsg->setName(mesgname);
  filename = spool_dir + mesgname + FILE_DATA_EXT;

  // creating lock-file for received message
  currMsg->lock();

  // opening/creating datafile
  store = new FileStorage(STORE_DATA, MODE_WRITE);
  ASSERT(store);
  if (store->open(filename.c_str()) == FALSE)
  {
    errors++;
    return(FALSE);
  }
  else
    log.add(4, "info: spoolfile '%s' successfully opened for writing", filename.c_str());

  // adding our received-line to file
  snprintf(buf, MAX_TRANS_SIZE,
    "Received: from %s (%s [%s])\r\n\tby %s (%s) with %s id %s\r\n\tfor <%s@%s>; %s\r\n",
    conn->remotehost.c_str(), conn->remotehost.c_str(), conn->remoteips.c_str(),
    conn->localhost.c_str(), fake?fake:LIMEZSTRING, "SMTP", mesgname.c_str(), currAdd->getUser(),
    currAdd->getDomain(), timer.getSMTPDate());

  raw = new RawData();
  ASSERT(raw);
  raw->data = (SBYTE *) buf;
  raw->size = strlen(buf);
  totalsize += raw->size;
  block.add(raw);

  if (store->add(&block) == FALSE)
  {
    errors++;
    log.add(1,"error: write on spool '%s' failed (%ld bytes)", filename.c_str(), raw->size);
  }

  // reading message
  while (errors < MAX_SMTP_ERRORS)
  {
    if (raw)
      delete raw;

    raw = new RawData();
    ASSERT(raw);
    raw->size = MAX_TRANS_SIZE;
    raw->data = (SBYTE *) buf;

    // clearing buffer NASTY HELP
    for (ULONG x=MAX_DATA_SIZE; x; x--)
      buf[x]='\0';

    if (!(len = conn->rawRead(raw)))
    {
      log.add(3,"error: read error on socket occured ...");
      errors++;
      break;
    }

    totalsize += len;

    // check size of incoming mail
    if (totalsize > config.getIntValue(KEY_SERVER_MAX_MAILSIZE))
    {
      // stop listening at 2 GB
      if (totalsize > MAX_VALID_MAIL_SIZE)
      {
        store->close();
        delete store;
        delete raw;
        return(FALSE);
      }

      if (pretending == FALSE)
      {
        log.add(2,"error: incoming message exceeds maximum size, will be deleted");
        store->close();
        delete store;

        if (unlink(filename.c_str()) == -1)
          log.add(1,"error: can't remove spool-file '%s', permission problems ?", filename.c_str());

        currMsg->unlock();
        pretending = TRUE;
      }
    }

    // check count of received lines in mail
    if (!strncasecmp(buf, "received:", 9))
    {
      if ((++received) > config.getIntValue(KEY_SERVER_MAX_HOPS))
      {
        log.add(2,"warning: incoming mail contains too many (%ld) received lines in header, rejected", received);
        if (pretending == FALSE)
        {
          store->close();
          delete store;
          pretending = TRUE;
        }
      }
    }

    // end receiving
    if ((len > 4) && (buf[len-5] == 0x0d) && (buf[len-4] == 0x0a) &&
        (buf[len-3] ==  '.') && (buf[len-2] == 0x0d) && (buf[len-1] == 0x0a))
      break;

    // in case we got different lines
    if ((clearline == TRUE) && (len == 3)  && (buf[len-3] ==  '.') &&
        (buf[len-2] == 0x0d) && (buf[len-1] == 0x0a))
      break;

    if ((buf[len-2] == 0x0d) && (buf[len-1] == 0x0a))
      clearline = TRUE;
    else
      clearline = FALSE;

    // write data into spoolfile
    if (pretending == FALSE)
    {
      block.add(raw);

      if (store->add(&block) == FALSE)
      {
        log.add(1,"error: write on spoolfile  '%s' during receive failed", filename.c_str());
        errors++;
        store->close();
        delete store;
        delete raw;
        return(FALSE);
      }
    }
  }
  delete raw;

  if (pretending == TRUE)
  {
    log.add(4,"info: receive of mail-DATA failed");
    return(FALSE);
  }
  else
  {
    store->close();
    delete store;

    // storing size in current message
    currMsg->setSize(totalsize);

    log.add(4,"info: receive of mail-DATA successfully finished (%s)", filename.c_str());
    snprintf(buf, MAX_DATA_SIZE, "250 %s message accepted for delivery\r\n", mesgname.c_str());
    conn->cwrite((SBYTE *) &buf);

    status = CON_DATA;
    return(TRUE);
  }
}

// denying incoming connection
BOOL Receiver::deny(VOID)
{
  DEBUGGER("Receiver::deny()");

  // installing signal handlers
  sigINT  = new SignalHandler(SIGINT, rec_signal);
  sigHUP  = new SignalHandler(SIGHUP, rec_signal);
  sigPIPE = new SignalHandler(SIGPIPE, rec_signal);
  sigCHLD = new SignalHandler(SIGCHLD, rec_signal);
  sigALRM = new SignalHandler(SIGALRM, rec_signal);
  
  writeWelcome();
  conn->cwrite(SMTP_421_ERRO);
  log.add(1,"error: maximum limit of tasks in scheduler reached, rejected");

  conn->cclose(TRUE);
  return(TRUE); 
}

// global signal handler (receiver)
static VOID rec_signal(INT sig)
{
  switch(sig)
  {
    case(SIGPIPE):
      log.add(1,"info: receiver got SIGPIPE (socket closed by remote), exiting");
      exit(0);
      break;

    case(SIGINT):
      log.add(1,"error: receiver got stop signal, exiting");
      exit(0);
      break;

    default:
      log.add(1,"error: receiver got unknown signal %d, ignored", sig);
      break;
  }
  return;
}
