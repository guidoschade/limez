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

#include "timer_def.h"

#include <unistd.h>
#include <math.h>

Timer::Timer()
{
  started = getUnix();
}

Timer::~Timer()
{
}

// return date string
SBYTE * Timer::getDate()
{
  gettimeofday(&ltime, &lzone);
  timestring = (SBYTE *) ctime((time_t*) &ltime);
  timestring[strlen(timestring)-1] = '\0';

  ASSERT(timestring);
  return (timestring);
}

// return uptime in seconds
ULONG Timer::uptime()
{
  return(getUnix()-started);
}

// get current time in seconds since 1970
ULONG Timer::getUnix()
{
  gettimeofday(&ltime, &lzone);
  ASSERT(ltime.tv_sec);
  return(ltime.tv_sec);
}

// return seconds since 1970 as string
SBYTE * Timer::getUnixString()
{
  snprintf(rdate, MAX_SMTP_DATE_SIZE, "%ld", getUnix());
  return((SBYTE *) &rdate);
}

// return reverse date (YYYYMMDDHHmmSS)
SBYTE * Timer::getRDate()
{
  gettimeofday(&ltime, &lzone);
  ltm = localtime((time_t*) &ltime);
  snprintf(rdate, MAX_SMTP_DATE_SIZE, "%04d%02d%02d%02d%02d%02d",
                  ltm->tm_year+1900, ltm->tm_mon+1, ltm->tm_mday,
                  ltm->tm_hour, ltm->tm_min, ltm->tm_sec);

  ASSERT((SBYTE *) &rdate);
  return((SBYTE *) &rdate);
}

// readable time with weekday and zone - offset
SBYTE * Timer::getSMTPDate()
{
  INT off, prtoff;
  time_t t;
  struct tm *lt = NULL, gmt;

  static const SBYTE day[7][4]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
  static const SBYTE month[12][4]={"Jan","Feb","Mar","Apr","May","Jun",
                                   "Jul","Aug","Sep","Oct","Nov","Dec"};
  time(&t); 
  gmt = *gmtime(&t);
  lt  = localtime((time_t*) &t);
  ASSERT(lt);

  off = (lt->tm_hour - gmt.tm_hour) * 60 + lt->tm_min - gmt.tm_min;

  if (lt->tm_year < gmt.tm_year)
    off -= 24 * 60;
  else if (lt->tm_year > gmt.tm_year)
    off += 24 * 60;
  else if (lt->tm_yday < gmt.tm_yday)
    off -= 24 * 60;
  else if (lt->tm_yday > gmt.tm_yday)
    off += 24 * 60;

  prtoff = (off/60) * 100;

  snprintf(rdate, MAX_SMTP_DATE_SIZE, "%s, %02d %s %04d %02d:%02d:%02d %s%04d",
          day[lt->tm_wday], lt->tm_mday, month[lt->tm_mon], lt->tm_year+1900,
          lt->tm_hour, lt->tm_min, lt->tm_sec, (prtoff>0)?"+":"-", abs(prtoff));

  ASSERT((SBYTE *) &rdate);
  return((SBYTE *) &rdate);
}

// return uptime - string / days/hours
SBYTE * Timer::uptimeString()
{
  ULONG x = uptime();
  FLOAT days = 0, hours = 0, mins = 0;

  if ((mins = (FLOAT) x / 60) >= 60)
  {
    if ((hours = mins / 60) >= 24)
    {
      days = hours / 24;
      hours = fmod(hours,24);
    }
    mins = fmod(mins, 60);
  }

  snprintf(rdate, MAX_SMTP_DATE_SIZE, "+%03d, %02d:%02d", (INT) days, (INT) hours, (INT) mins);
  return((SBYTE *) &rdate);
}
