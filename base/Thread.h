//
// Created by Kukai on 2020/12/7.
//

#ifndef MUDUO_BASE_THREAD_H
#define MUDUO_BASE_THREAD_H

#include "Atomic.h"
#include "../boost/noncopyable.h"

#include <functional>
#include <memory>
#include <pthread.h>

namespace Kukai{
    /*
     *线程类的封装
     */
    class Thread : boost::noncopyable{
    public:
        typedef std::function<void ()> ThreadFunc;

        /*
         *构造函数，调用setDefaultName，用于各种成员传入参数
         **/
        explicit Thread(ThreadFunc, const std::string& name = std::string());

        /*
         * 析构函数负责把线程1detach让其自己销毁
         **/
        ~Thread();

        /*
         * 发送信号开启线程
         * */
        void start();

        /*
         * 回收线程
         * */
        void join();

        /*
         * 返回内部成员线程是否启动，线程ID，线程名字
         * */
        bool started() const { return started_;}
        pid_t tid() const {return tid_;}
        const std::string&name() const { return name_;}

        // 返回本进程创建的线程数目
        static int numCreated() { return numCreated_.get();}

    private:
        bool         started_;          // 线程是否启动
        bool         joined_;           // 是否被join回收
        pthread_t    pthreadId_;        // 线程ID
        pid_t        tid_;              // 进程ID
        ThreadFunc   func_;             // 线程函数
        std::string  name_;             // 线程名字

        static AtomicInt32 numCreated_; // 本进程创建的线程数量
    };
}   //namespace Kukai

#endif //MUDUO_BASE_THREAD_H
