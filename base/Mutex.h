//
// Created by Kukai on 2020/12/21.
//

#ifndef MUDUO_BASE_MUTEX_H
#define MUDUO_BASE_MUTEX_H

#include "Thread.h"
#include "../boost/noncopyable.h"
#include "CurrentThread.h"

#include <cassert>
#include <pthread.h>

namespace Kukai{
    class MutexLock{
    public:
        MutexLock() : holder_(0){
            pthread_mutex_init(&mutex_, NULL);
        }
        ~MutexLock(){
            assert(holder_ == 0);
            pthread_mutex_destroy(&mutex_);
        }

        bool isLockedByThisThread(){
            return holder_ == CurrentThread::tid();
        }

        void assertLocked(){
            assert(isLockedByThisThread());
        }

        void lock(){
            pthread_mutex_lock(&mutex_);
            holder_ == CurrentThread::tid();
        }

        void unlock(){
            holder_ = 0;
            pthread_mutex_unlock(&mutex_);
        }

        pthread_mutex_t *getPthreadMutex(){
            return &mutex_;
        }

    private:
        pthread_mutex_t mutex_;
        pid_t holder_;
    };

    class MutexLockGuard : boost::noncopyable {
    public:
        explicit MutexLockGuard(MutexLock &mutex) : mutex_(mutex){
            mutex_.lock();
        }
        ~MutexLockGuard(){
            mutex_.unlock();
        }

    private:
        MutexLock &mutex_;
    };
}

#define MutexLockGuard(x) error "Missing guard object name"

#endif //MUDUO_BASE_MUTEX_H
