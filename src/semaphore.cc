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

#include "semaphore_def.h"

extern Log log;

Semaphore::Semaphore()
:semaphoreId(0),semaphoreKey(0),semaphoreOps(NULL)
{
}

Semaphore::~Semaphore()
{
  if (semaphoreOps)
    delete semaphoreOps;
}

// initialize semaphore
inline BOOL Semaphore::init(VOID)
{
  log.add(4,"info: initializing semaphore");
  ASSERT(!semaphoreKey);

  semaphoreKey = (key_t) getpid();
  semaphoreOps = new (struct sembuf)[2*sizeof(struct sembuf)];

  // getting semaphore
  if ((semaphoreId = semget(semaphoreKey, 1, IPC_CREAT | 0666)) == -1)
  {
    log.add(2,"error: failed to get semaphore (%s)", strerror(errno));
    return(FALSE);
  } 
  log.add(4,"info: semaphore successfully allocated");
  return(TRUE);
}

// locking/waiting for semaphore
inline BOOL Semaphore::lock(VOID)
{
  ASSERT(semaphoreId);
  ASSERT(semaphoreOps);

  semaphoreOps[0].sem_num = 0;
  semaphoreOps[0].sem_op = 0;
  semaphoreOps[0].sem_flg = SEM_UNDO;
        
  semaphoreOps[1].sem_num = 0;
  semaphoreOps[1].sem_op = 1;
  semaphoreOps[1].sem_flg = SEM_UNDO | IPC_NOWAIT;

  if (semop(semaphoreId, semaphoreOps, 2) == -1)
  {
    log.add(2,"error: semaphore operation failed (%s)", strerror(errno));
    return(FALSE);
  } 
  log.add(4,"info: entering critical section");
  return(TRUE);
}

// unlocking semaphore after critical section
inline BOOL Semaphore::unlock(VOID)
{
  ASSERT(semaphoreId);
  ASSERT(semaphoreOps);

  semaphoreOps[0].sem_num = 0;
  semaphoreOps[0].sem_op = -1;
  semaphoreOps[0].sem_flg = SEM_UNDO | IPC_NOWAIT;

  if (semop(semaphoreId, semaphoreOps, 1) == -1)
  {
    log.add(2,"error: semaphore operation failed (%s)", strerror(errno));
    return(FALSE);
  } 
  return(TRUE);
}

// removing semaphore
inline BOOL Semaphore::remove(VOID)
{
  log.add(4,"info: deleting semaphore");

  if (!semaphoreId)
    return(FALSE);
 
  if (semctl(semaphoreId, 0, IPC_RMID, NULL) == -1) 
  {
    log.add(2,"error: semaphore deletion failed (%s)", strerror(errno));
    return(FALSE);
  } 

  delete semaphoreOps;
  semaphoreKey = 0;
  semaphoreId = 0;
  log.add(4,"info: successfully deleted semaphore");
  return(TRUE);
}
