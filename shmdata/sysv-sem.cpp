/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 */

#include "./sysv-sem.hpp"
#include <errno.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <mutex>

using namespace std::chrono_literals;

namespace shmdata {

bool force_semaphore_cleaning(key_t key, AbstractLogger* log) {
  auto semid = semget(key, 2, 0);
  if (semid < 0) {
    int err = errno;
    log->debug("semget (forcing semaphore cleaning): %", strerror(err));
    return false;
  }
  if (semctl(semid, 0, IPC_RMID, 0) != 0) {
    int err = errno;
    log->error("semctl removing semaphore %", strerror(err));
  }
  return true;
}

namespace semops {
// sem_num 0 is for reading, 1 is for writer
static struct sembuf read_start[] = {{1, 0, 0}};   // wait writer
static struct sembuf read_end[] = {{0, -1, 0}};    // decr reader
static struct sembuf write_start[] = {{0, 0, 0},   // wait reader is 0
                                      {1, 1, 0},   // incr writer
                                      {0, 1, 0}};  // incr reader
static struct sembuf write_end[] = {{0, -1, 0},    // decr reader
                                    {1, -1, 0}};   // decr writer

// thanks https://tldp.org/LDP/lpg/node53.html
union semun {
  int val;                /* value for SETVAL */
  struct semid_ds *buf;   /* buffer for IPC_STAT & IPC_SET */
  ushort *array;          /* array for GETALL & SETALL */
  struct seminfo *__buf;  /* buffer for IPC_INFO */
  void *__pad;
};
}  // namespace semops

sysVSem::sysVSem(key_t key, AbstractLogger* log, bool owner, mode_t unix_permission)
    : key_(key),
      owner_(owner),
      semid_(semget(key_, 2, owner ? IPC_CREAT | IPC_EXCL | unix_permission : 0)),
      log_(log) {
  if (semid_ < 0) {
    int err = errno;
    log_->debug("semget: %", strerror(err));
    return;
  }
}

sysVSem::~sysVSem() {
  if (is_valid() && owner_) {
    if (semctl(semid_, 0, IPC_RMID, 0) != 0) {
      int err = errno;
      log_->error("semctl removing semaphore %", strerror(err));
    }
  }
}

void sysVSem::cancel_commited_reader() {
  if (-1 == semop(semid_, semops::read_end, sizeof(semops::read_end) / sizeof(*semops::read_end))) {
    int err = errno;
    log_->error("semop cancel: %", strerror(err));
  }
}

bool sysVSem::is_valid() const { return 0 < semid_; }

ReadLock::ReadLock(sysVSem* sem) : sem_(sem) {
  if (-1 == semop(sem_->semid_,
                  semops::read_start,
                  sizeof(semops::read_start) / sizeof(*semops::read_start))) {
    int err = errno;
    sem_->log_->debug("semop ReadLock %", strerror(err));
    valid_ = false;
  }
}

ReadLock::~ReadLock() {
  if (is_valid())
    semop(sem_->semid_, semops::read_end, sizeof(semops::read_end) / sizeof(*semops::read_end));
}

WriteLock::WriteLock(sysVSem* sem) : sem_(sem) {

  std::mutex cv_m;
  std::condition_variable cv;
  bool got_semaphore_in_a_reasonable_time = false;

  // this is a safeguard against readers crashing in the middle of their read callback. It resets
  // the reader semaphore if a second has elapsed before all the readers have read the last written data.
  std::thread read_semaphore_reset_thread([sem_ = this->sem_, &got_semaphore_in_a_reasonable_time, &cv_m, &cv]() {

    std::unique_lock<std::mutex> lk(cv_m);
    // wait for 1000ms or until the rest of the constructor assures us
    // that it could continue. A timeout of 1000ms is arbitrary but should be long enough
    // to only occur in truly problematic cases.
    cv.wait_for(lk, 1000ms, [&] {return got_semaphore_in_a_reasonable_time;});

    // If we exited the wait loop without having been notified that all the semaphore operations are done,
    // it is because we are stuck waiting for one or more commited readers to decrement the first semaphore. This is probably
    // because they have crashed. It is not reasonnable to wait forever so we reset the semaphore to 0 and let the writer
    // continue its job.
    if (!got_semaphore_in_a_reasonable_time) {
      semops::semun params;
      params.val = 0;
      semctl(sem_->semid_, 0, SETVAL, params);
    }
  });
  // waits to do the required semaphore operations to have the "write lock".
  // semops::write_start defines three operations that will be applied on two semaphores.
  // The first operation is to wait for the first semaphore to fall to zero, meaning that all
  // the commited readers had time to read the last data.
  // the second operation is to increment the second semaphore to indicate to the readers that the
  // writer is currently writing.
  // The third operation is toincrement the reader semaphore
  // (probably to stop another writer to start writing at the same time though
  // its not clear to me why we would need that).
  auto result = semop(sem_->semid_,
                      semops::write_start,
                      sizeof(semops::write_start) / sizeof(*semops::write_start));
  {
    std::lock_guard lk(cv_m);
    got_semaphore_in_a_reasonable_time = true;
  }
  cv.notify_one();
  read_semaphore_reset_thread.join();
  if (-1 == result) {
    int err = errno;
    sem_->log_->error("semop WriteLock: %", strerror(err));
    valid_ = false;
  }
}

bool WriteLock::commit_readers(short num_reader) {
  struct sembuf read_commit_reader[] = {{0, num_reader, 0}};
  if (-1 == semop(sem_->semid_,
                  read_commit_reader,
                  sizeof(read_commit_reader) / sizeof(*read_commit_reader))) {
    int err = errno;
    sem_->log_->error("semop commit readers: %", strerror(err));
    return false;
  }
  return true;
}
WriteLock::~WriteLock() {
  if (!is_valid()) return;
  semop(sem_->semid_, semops::write_end, sizeof(semops::write_end) / sizeof(*semops::write_end));
}

}  // namespace shmdata
