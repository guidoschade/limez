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

#ifndef GLOBAL_H
#define GLOBAL_H 

// provide assertions
#include <assert.h>
#include <string>

#ifdef USE_MSQL_DB
  #define USE_DATABASE
#endif
#ifdef USE_MYSQL_DB
  #define USE_DATABASE
#endif
#ifdef USE_POSTGRES_DB
  #define USE_DATABASE
#endif

// define raw datatypes
enum BOOL {FALSE = 0, TRUE = 1};

#define UBYTE unsigned char
#define UWORD unsigned short
#define ULONG unsigned long

#define SBYTE char
#define SWORD short
#define SLONG long
#define INT   int

#define VOID   void
#define FLOAT  float
#define DOUBLE double
#define STRING std::string
#define STRIDX std::string::size_type

#ifndef NULL
 #define NULL 0
#endif

// debugging on/off
#define LIMEZ_DEBUG         TRUE

#ifdef LIMEZ_DEBUG
 #define ASSERT(x)          assert(x)
 #define DEBUGGER(x)        log.add(7,x)
 #define MLDEBUGGER(x)      mllog.add(7,x)
#else
 #define ASSERT(x)          //(x)
 #define DEBUGGER(x)        //(7,x)
 #define MLDEBUGGER(x)      //(7,x)
#endif

// global defines
#define VERSION            "1.0-beta-4"
#define LIMEZSTRING        "Limez " VERSION
#define LIMEZURL           "http://www.limez.net/"
#define DEBUG_MAX          9
#define USE_DB_STORAGE     "database"
#define USE_LDAP_STORAGE   "ldap"
#define FILE_DATA_EXT      ".data"
#define FILE_CONF_EXT      ".config"
#define FILE_LOG_EXT       ".log"
#define FILE_LOCK_EXT      ".lock"
#define FILE_DNS_CACHE     "cache.bin"
#define DEFAULT_LDAP_PORT  389

// scheduler + tasks
#define MAX_TASK_TIME              43200   // max 12 hours lifetime of task (max time for broadcast)
#define MAX_SMTP_ERRORS                5   // 5 errors until exiting smtp-connection
#define MAX_USERS_ONCE               256   // max users to get from storage once
#define MAX_TRANSCEIVER_TIME        1800   // max 30 mins lifetime of transceiver
#define MAX_INT_STR_SIZE              16   // max size of string containing integer
#define MAX_LDAP_ENTRIES              64   // max number of entries to get from ldap-server
#define MAX_LDAP_WAIT_TIME            30   // 30 seconds to wait for ldap answer
#define MAX_LDAP_FILTER_SIZE         256   // size of ldap search filter string

// resolver
#define IPV4_DEC_DOT_SIZE             18   // num of bytes for dottet decimal 000.000.000.000

// log
#define LOG_BUFSIZE                  256
#define LOG_ENTRYSIZE     LOG_BUFSIZE+32   // max length of logentry
#define MAX_LINE_SIZE               1024   // maximum length of a single line

// database
#define SQL_BUF_SIZE                 512   // size of sql - buffer needed for database

// types for DataBlock / Storages
enum BLOCKTYPE  {TYPE_RAW, TYPE_LINE, TYPE_ADDRESS, TYPE_KEYVAL};
enum STORECLASS {STORE_LOG, STORE_CONF, STORE_USER, STORE_DATA, STORE_SENDER};
enum FILEMODE   {MODE_READ, MODE_WRITE, MODE_APPEND, MODE_DELETE, MODE_CHECK};

// config value types
enum CONFOPT    {CONF_NONE = 0x00, CONF_UNIQ = 0x01, CONF_MULT = 0x02,
                 CONF_MAND = 0x04, CONF_PROT = 0x08, CONF_TEMP = 0x10, CONF_PROTTEMP = 0x18,
                 CONF_NOEDIT = 0x20};

enum CONFTYPE   {TYPE_STR, TYPE_XINT, TYPE_BOOL};

// connection status
enum CONNTYPE {CONN_INIT, CONN_LIST, CONN_OPEN};
enum CONSTATE {CON_NONE = 0, CON_HELO = 1, CON_FROM = 2, CON_RCPT = 3, CON_DATA = 4, CON_DONE = 5};

// smtp commands (just added lowercase ascii * multiplier)
enum SMTP_CMD {EHLO = 122941, HELO = 122914, HELP = 123914, SEND = 112125, SAML = 119985, SOML = 120125,
               RSET = 127364, QUIT = 127783, TURN = 122686, MAIL = 119579, DATA = 109670, RCPT = 128304,
               EXPN = 122501, LRLD = 112048, LCMD = 111998, LPWD = 113128, VRFY = 132458, NOOP = 124320,
               LSHW = 130658, LADD = 111078, LDEL = 119208};

enum CMDTYPE {CMD_ADDUSER, CMD_DELUSER, CMD_ADDSENDER, CMD_DELSENDER};

// scheduler / task
enum JOB {MODE_RECEIVE, MODE_TRANSMIT, MODE_PROCESS};

// address recipient types
enum RECTYPE {REC_NONE = 0x00, REC_PROG = 0x10, REC_LIST = 0x20, REC_NOTSET = 0x40};

enum RESTYPE {NO_RESOLVE, TRY_RESOLVE};
enum QOPTIONS {Q_NONE = 0x00, Q_INIT = 0x01, Q_AGAIN = 0x02, Q_DONE = 0x04};

// config
#define MAX_KEY_SIZE             64  // max length of key in configfile
#define MAX_VAL_SIZE            128  // max length of value in config
#define MAX_KEY_VAL_LINE_SIZE    MAX_KEY_SIZE + MAX_VAL_SIZE + 5

// message + line
#define MAX_MAIL_ERRORS           7  // max wrong mail commands
#define MAX_MAIL_COMMANDS       256  // max number of commands in mail
#define MAX_MAIL_COMMAND_SIZE   132  // max length of single mail command

// address
#define MAX_FIRSTNAME_SIZE       64  // firstname in db and file
#define MAX_LASTNAME_SIZE        64  // lastname
#define MAX_TITLE_SIZE           64  // title
#define MAX_SMTP_USER_SIZE       64  // username, part before @
#define MAX_SMTP_DOMAIN_SIZE     64  // domain, part after @
#define MAX_SPOOL_FILENAME_SIZE  64  // name of spoolfile
#define MAX_PASS_SIZE            16  // maximum size of password
#define MAX_PACKET_SIZE        8192  // resolver-query-buffer

#define MAX_SMTP_ADDR_SIZE     MAX_SMTP_USER_SIZE+1+MAX_SMTP_DOMAIN_SIZE
#define MIN_SMTP_ADDR_SIZE        5  // minimum size of address
#define MAX_SMTP_DATE_SIZE       32  // max size of Datestring

// transmitter/receiver
#define MAX_VALID_MAIL_SIZE    2147483646 // hardlimit of 2 GB
#define MAX_DATA_SIZE          4096  // max DATA- buffer (buf) size
#define SEND_SIZE              1000  // bytes used to send SMTP-mails
#define MAX_TRANS_SIZE         1024  // max size of SMTP-command
#define SHOW_USER_ONCE           20  // number of users sent to admin

#define BROADCAST_SLEEP_TIME      5  // seconds to wait til new slots are open

// smtp status / error messages
#define SMTP_214_HELP   "214 help not available, winners HELP themselves\r\n"
#define SMTP_250_OKAY   "250 OK\r\n"
#define SMTP_250_RSET   "250 Reset state\r\n"
#define SMTP_250_DATA   "250 Message accepted for delivery\r\n"
#define SMTP_252_VRFY   "252 Cannot VRFY user\r\n"
#define SMTP_354_DATA   "354 Enter mail, end with '.' on a line by itself\r\n"
#define SMTP_421_ERRO   "421 Too many incoming connections, please try again later\r\n"
#define SMTP_500_UNRE   "500 Command unrecognized\r\n"
#define SMTP_501_HELO   "501 HELO requires domain address\r\n"
#define SMTP_501_SYNT   "501 Syntax error\r\n"
#define SMTP_501_NOAU   "501 Not authorized\r\n"
#define SMTP_502_NOOP   "502 Sorry, we do not allow this operation\r\n"
#define SMTP_503_SEQ1   "503 Need HELO before MAIL\r\n"
#define SMTP_503_SEQ2   "503 Need MAIL before RCPT\r\n"
#define SMTP_503_SEQ3   "503 Need MAIL command\r\n"
#define SMTP_503_SEQ4   "503 Need RCPT (recipient)\r\n"
#define SMTP_503_SEQ5   "503 Sender already specified\r\n"
#define SMTP_550_ERRO   "550 Requested action not taken: no such user/list\r\n"
#define SMTP_552_ABRT   "552 Requested mail action aborted: exceeded storage allocation\r\n"
#define SMTP_553_ERRO   "553 Requested action not taken: no such user/list\r\n"
#define SMTP_554_ERRO   "554 Transaction failed\r\n"

// config keys
#define KEY_DB_HOST                "db_host"
#define KEY_DB_NAME                "db_name"
#define KEY_DB_USER                "db_user"
#define KEY_DB_PASS                "db_pass"

#define KEY_LDAP_HOSTS             "ldap_hosts"
#define KEY_LDAP_USER              "ldap_user"
#define KEY_LDAP_PASS              "ldap_pass"
#define KEY_LDAP_FILTER            "ldap_filter"
#define KEY_LDAP_BASE_DN           "ldap_base_dn"

#define KEY_LOGFILE                "logfile"
#define KEY_MAILINGLIST_DB         "mailinglist_db"
#define KEY_MAILINGLIST            "mailinglist"
#define KEY_DEBUG_LEVEL            "debug_level"
#define KEY_DEBUG_TEST_ONLY        "debug_test_only"
#define KEY_DEBUG_CONNECT_ONLY     "debug_connect_only"

#define KEY_SERVER_ROOT            "server_root"
#define KEY_SERVER_DAEMONIZE       "server_daemonize"
#define KEY_SERVER_SPOOL_ONLY      "server_spool_only"
#define KEY_SERVER_SEND_HOST       "server_send_host"
#define KEY_SERVER_BACKLOG         "server_backlog"
#define KEY_SERVER_ADMIN           "server_admin"
#define KEY_SERVER_ALIAS           "server_alias"
#define KEY_SERVER_HOST            "server_host"
#define KEY_SERVER_PORT            "server_port"
#define KEY_SERVER_REMOTE_PORT     "server_remote_port"
#define KEY_SERVER_TIMEOUT         "server_timeout"
#define KEY_SERVER_MAIL            "server_mail"
#define KEY_SERVER_DOMAIN          "server_domain"
#define KEY_SERVER_UID             "server_uid"
#define KEY_SERVER_GID             "server_gid"
#define KEY_SERVER_STRING          "server_string"
#define KEY_SERVER_SUBJECT         "server_subject"
#define KEY_SERVER_CONFIG          "server_config"
#define KEY_SERVER_MAX_PROCS       "server_max_procs"
#define KEY_SERVER_CACHETIME       "server_cachetime"
#define KEY_SERVER_MAX_HOPS        "server_max_hops"
#define KEY_SERVER_WEB_PASS        "server_web_pass"
#define KEY_SERVER_WEB_HOSTS       "server_web_hosts"
#define KEY_SERVER_QUEUE_INTERVAL  "server_queue_interval"
#define KEY_SERVER_SPOOL_INTERVAL  "server_spool_interval"
#define KEY_SERVER_MAX_QUEUETIME   "server_max_queuetime"
#define KEY_SERVER_MAX_QUEUETRIES  "server_max_queuetries"
#define KEY_SERVER_MAX_MAILSIZE    "server_max_mailsize"
#define KEY_SERVER_MAX_SENDPROCS   "server_max_sendprocs"
#define KEY_SERVER_SPOOL_DIR       "server_spool_dir"
#define KEY_SERVER_DONE_DIR        "server_done_dir"
#define KEY_SERVER_FAKE_NAME       "server_fake_name"
#define KEY_SERVER_COMMAND_QUIET   "server_command_quiet"

#define KEY_LIST_MAX_SEND_TASKS    "list_max_send_tasks"
#define KEY_LIST_MAX_MAILSIZE      "list_max_mailsize"
#define KEY_LIST_NAME              "list_name"
#define KEY_LIST_PASS              "list_pass"
#define KEY_LIST_CONFIG            "list_config"
#define KEY_LIST_SEND              "list_send"
#define KEY_LIST_DESCR             "list_descr"
#define KEY_LIST_SUBSCRIBE         "list_subscribe"
#define KEY_LIST_UNSUBSCRIBE       "list_unsubscribe"
#define KEY_LIST_STAT              "list_stat"
#define KEY_LIST_INFO              "list_info"
#define KEY_LIST_ADMIN             "list_admin"
#define KEY_LIST_DOMAIN            "list_domain"
#define KEY_LIST_USER_SOURCE       "list_user_source"
#define KEY_LIST_SENDER_SOURCE     "list_sender_source"
#define KEY_LIST_WELCOME           "list_welcome"
#define KEY_LIST_ME_TOO            "list_me_too"
#define KEY_LIST_INFORM_ADMIN      "list_inform_admin"
#define KEY_LIST_GOODBYE           "list_goodbye"
#define KEY_LIST_VERBOSE           "list_verbose"
#define KEY_LIST_HEADER_FROM       "list_header_from"
#define KEY_LIST_HEADER_RECIPIENT  "list_header_recipient"
#define KEY_LIST_HEADER_SENDER     "list_header_sender"
#define KEY_LIST_AUTO_UNSUB        "list_auto_unsub"
#define KEY_LIST_DISABLE_SPOOL     "list_disable_spool"
#define KEY_LIST_HEADER_REPLY      "list_header_reply"
#define KEY_LIST_HEADER_SUBJECT    "list_header_subject"
#define KEY_LIST_USERTABLE_NAME    "list_usertable_name"
#define KEY_LIST_HEADER_FILE       "list_header_file"
#define KEY_LIST_FOOTER_FILE       "list_footer_file"
#define KEY_LIST_LDAP_USER_GROUP   "list_ldap_user_group"
#define KEY_LIST_LDAP_SENDER_GROUP "list_ldap_sender_group"

#define KEY_MESG_RCPT              "mesg_rcpt"
#define KEY_MESG_FROM              "mesg_from"
#define KEY_MESG_STAT              "mesg_stat"
#define KEY_MESG_NAME              "mesg_name"
#define KEY_MESG_TRIES             "mesg_tries"
#define KEY_MESG_SIZE              "mesg_size"
#define KEY_MESG_SEND              "mesg_send"
#define KEY_MESG_INIT_DATE         "mesg_init_date"
#define KEY_MESG_CHNG_DATE         "mesg_chng_date"
#define KEY_MESG_ORIG              "mesg_orig"
#define KEY_MESG_LIST_NAME         "mesg_list_name"

#define KEY_LOCK_PID               "lock_pid"
#define KEY_LOCK_DATE              "lock_date"

typedef struct {
  SBYTE name[MAX_SMTP_DOMAIN_SIZE+1];
  ULONG ip1, ip2, ip3, cdat;
} cache_entry;

extern SBYTE * getNextId(VOID);

#endif //GLOBAL_H
