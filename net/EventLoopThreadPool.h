//
// Created by Kukai on 2021/1/14.
//

#ifndef MUDUO_NET_EVENTLOOPTHREADPOOL_H
#define MUDUO_NET_EVENTLOOPTHREADPOOL_H

#include <vector>
#include <functional>
#include <memory>

#include "../boost/noncopyable.h"

namespace Kukai{

    class EventLoop;
    class EventLoopThread;

    class EventLoopThreadPool : boost::noncopyable {
    public:
        EventLoopThreadPool(EventLoop *baseLoop);
        ~EventLoopThreadPool();

        void setThreadPool(int numThreads) {
            numThreads_ = numThreads;
        }

        void setThreadNum(int numThreads){
            numThreads_ = numThreads;
        }

        void start();

        EventLoop *getNextLoop();

    private:
        EventLoop *baseLoop_;
        bool started_;
        int numThreads_;
        int next_;
        std::vector<std::unique_ptr<EventLoopThread>> threads_;
        std::vector<EventLoop*> loops_;
    };
}


#endif //MUDUO_NET_EVENTLOOPTHREADPOOL_H
