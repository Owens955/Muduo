//
// Created by Kukai on 2020/12/21.
//

#include "EventLoopThread.h"

#include "EventLoop.h"

#include <functional>

using namespace Kukai;

/*
 * 构造函数，负责初始化数据成员，注意threadFunc
 * */
EventLoopThread::EventLoopThread()
    : loop_(nullptr),
      exiting_(false),
      thread_(std::bind(&EventLoopThread::threadFunc, this)),
      mutex_(),
      cond_(mutex_)
      {   }

 /*
  * 析构函数，eventloop::quit，thread::join()
  * */
EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    loop_->quit();
    thread_.join();
}

/*
 * 获取内部eventloop对象的地址
 * */
EventLoop *EventLoopThread::startLoop() {
    assert(!thread_.started());
    thread_.start();    //  创建线程调用threadFunc()函数

    {
        // 条件变量保证eventloop创建完成，loop被赋值
        MutexLockGuard lock(mutex_);
        while (loop_ == nullptr){
            cond_.wait();
        }
    }

    return loop_;
}

/*
 * 线程函数体，负责创建eventloop对象，以及调用eventloop::loop()
 * */
void EventLoopThread::threadFunc() {
    EventLoop loop;
    // 可调用回调函数
    {
        // 通知startLoop()eventloop创建完成
        MutexLockGuard lock(mutex_);
        loop_ = &loop;
        cond_.notify();
    }
    loop.loop();
    // 循环体结束，eventloop析构，把loop设为空
}