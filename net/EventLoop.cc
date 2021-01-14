//
// Created by Kukai on 2020/12/7.
//

#include "EventLoop.h"

#include "../base/Log.h"
#include "Channel.h"
#include "Poller.h"
#include "TimerQueue.h"

#include <iostream>
#include <cassert>
#include <sys/eventfd.h>
#include <unistd.h>

using namespace Kukai;

__thread EventLoop *t_loopInThisThread = nullptr; // 每个eventloop线程拥有的那个eventloop*
const int kPollTimeMs = 10000;              // poller::poll()的等待时间，超过就返回

static int createEventfd(){
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0){
        std::cout << " Failed in eventfd " << std::endl;
        abort();
    }
    return evtfd;
}

/*
 * 构造函数，初始化成员，设置当前线程的t_loopInThisThread为this
 * */
EventLoop::EventLoop()
        : looping_(false),
          quit_(false),
          callingPendingFunctors_(false),
          threadId_(CurrentThread::tid()),
          poller_(new Poller(this)),
          timerQueue_(new TimerQueue(this)),
          wakeupFd_(createEventfd()),
          wakeupChannel_(new Channel(this, wakeupFd_)){
    LOG("EventLoop created " << this << " in thread " << threadId_);

    // 是否重复创建eventloop
    if (t_loopInThisThread) {
        LOG("Another EventLoop " << t_loopInThisThread
                                 << " exists in this thread " << threadId_);
    } else {
        t_loopInThisThread = this;
    }
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead,this));
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
    assert(!looping_);
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

/*
 * 主要三块：
 * 1.poller::poll()
 * 2.channel::handleEvent()
 * 3.处理用户任务队列
 * */
void EventLoop::loop() {
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;

    while (!quit_){
        // 调用poller::poll()获取发生网络事件的套接字
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (ChannelList::iterator it = activeChannels_.begin();
             it != activeChannels_.end(); ++it)
        {
            (*it)->handleEvent(pollReturnTime_);
        }
        doPendingFunctors();
    }

    LOG("EventLoop " << this << " stop looping ");
    looping_ = false;
}

/*
 * 停止eventloop::loop()
 * */
 void EventLoop::quit(){
        quit_ = true;
        if ( !isInLoopThread()){
            wakeup();
        }
}

void EventLoop::runInLoop(const Functor &cb) {
    if (isInLoopThread()){
        cb();
    } else{
        queueInLoop(cb);
    }
 }

 void EventLoop::queueInLoop(const Functor &cb) {
     {
         MutexLockGuard lock(mutex_);
         pendingFuntors_.push_back(cb);
     }

     if (!isInLoopThread() || callingPendingFunctors_){
         wakeup();
     }
 }

/*
 * 定时器相关，设定一个定时器用户执行任务，把定时器任务加入到timerQueue中
 * */
Timerld EventLoop::runAt(const Timestamp &time, const TimerCallback &cb) {
    return timerQueue_->addTimer(cb, time, 0.0);
}

Timerld EventLoop::runAfter(double delay, const TimerCallback &cb) {
    Timestamp time(addTime(Timestamp::now(), delay));
    return runAt(time, cb);
}

Timerld EventLoop::runEvery(double interval, const TimerCallback &cb) {
    Timestamp time(addTime(Timestamp::now(), interval));
    return timerQueue_->addTimer(cb, time, interval);
}

/*
 * 更新channel，实际上调用了poller::updatechannel，更新poller的pollfds数组
 * */
void EventLoop::updateChannel(Channel *channel) {
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->updateChannel(channel);
    }

void EventLoop::removeChannel(Channel *channel) {
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->removeChannel(channel);
}

/*
 * 断言，当前线程就是eventloop所在的线程
 * */
void EventLoop::abortNotInLoopThread() {
    std::cout << " EventLoop::abortNotInLoopThread - EventLoop " << this
              << " was created in threadId_ = " << threadId_
              << ", current thread id = " << CurrentThread::tid() << std::endl;
}

void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one){
        std::cout << " EventLoop::wakeup() writes " << n << " bytes instead of 8 " << std::endl;
    }
}

void EventLoop::handleRead(){
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one){
        std::cout << " EventLoop::wakeup() reads " << " bytes instead of 8 " << std::endl;
    }
}

void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFuntors_);
    }

    for (size_t i = 0; i < functors.size() ; ++i) {
        functors[i]();
    }
    callingPendingFunctors_ = false;
}
