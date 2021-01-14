//
// Created by Kukai on 2020/12/21.
//

#ifndef MUDUO_NET_EVENTLOOPTHREAD_H
#define MUDUO_NET_EVENTLOOPTHREAD_H

#include "../base/Thread.h"
#include "../base/Mutex.h"
#include "../base/Condition.h"

namespace Kukai{
    class EventLoop;

    class EventLoopThread : boost::noncopyable {
    public:
        /*
         * 构造函数，负责成员变量的初始化注意callback设置为cb
         * thread会被用evevtloopthread::threadFunc()初始化，执行threadFunc()
         * */
        EventLoopThread();
        /*
         * 析构函数，负责停止eventloop，回收thread
         * */
        ~EventLoopThread();
        /*
         * 返回一个创建好的eventloop*，负责启动线程调用threadFunc()
         * */
        EventLoop *startLoop();

    private:
        /*
         * eventloop线程函数，在sraLoop中被调用
         * 负责创建一个eventloop对象，设置loop，执行用户传入的回调ThreadInitCallback等
         * */
        void threadFunc();

        EventLoop *loop_;   // 线程内部的eventloop*
        bool exiting_;      // 线程是否退出
        Thread thread_;     // 线程
        MutexLock mutex_;   // 互斥锁
        Condition cond_;    // 条件变量
    };
}

#endif //MUDUO_NET_EVENTLOOPTHREAD_H
