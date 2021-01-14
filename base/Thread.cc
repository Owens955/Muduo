//
// Created by Kukai on 2020/12/7.
//

#include "Thread.h"
#include "CurrentThread.h"

#include <type_traits>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>
#include <memory>

namespace Kukai
{
    namespace detail {

        /*
         * 得到唯一的线程ID
         * */
        pid_t gettid() {
            return static_cast<pid_t>(::syscall(SYS_gettid));
        }
        /*
         * currentthread中以前定义了四个__thread变量，用来描述线程信息，在这里进行设置
         * */
        void afterFork(){
            Kukai::CurrentThread::t_cachedTid = gettid();
            Kukai::CurrentThread::t_threadName = "main";
        }

        /*
         * 线程初始化函数，负责初始化线程信息
         * */
        class ThreadNameInitializer{
        public:
            ThreadNameInitializer(){
                Kukai::CurrentThread::t_threadName = "main";
                pthread_atfork(nullptr, nullptr, &afterFork);
            }
        };
        // 全局线程初始化对象，完成currentthread中线程全局变量的设置
        ThreadNameInitializer init;

        /*
         * 线程数据类
         * */
        struct ThreadData {
            typedef Kukai::Thread::ThreadFunc ThreadFuc;
            ThreadFuc func_;
            std::string name_;
            pid_t *tid_;
            std::weak_ptr<pid_t> wkTid_;

            // 构造函数初始化
            ThreadData(const ThreadFuc &func,
                       const std::string &name,
                       pid_t *tid)
                    : func_(func),
                      name_(name),
                      tid_(tid) {}

            /*
             * 运行线程
             * */
            void runInThread() {
                // 设置类成员变量
                pid_t tid = Kukai::CurrentThread::tid();
                std::shared_ptr<pid_t> ptid = wkTid_.lock();

                if (ptid) {
                    *ptid = tid;
                    ptid.reset();
                }

                // 设置线程名字
                Kukai::CurrentThread::t_threadName = name_.empty() ? "muduoThread" : name_.c_str();
                ::prctl(PR_SET_NAME, Kukai::CurrentThread::t_threadName);
                // 具体的运行函数
                func_();
                // 执行结束
                Kukai::CurrentThread::t_threadName = "finished";
            }
        };

            /*
             * 从属于detail命名空间，使用ThreadData类创建并且启动线程
             * */
            void *startThread(void *obj) {
                ThreadData *data = static_cast<ThreadData *>(obj);
                data->runInThread();
                delete data;
                return NULL;
            }
    }   // namespace detail

    /*
     * 之前currentthread中没有定义此方法，因此在这里进行定义
     * */
    pid_t CurrentThread::cacheTid() {
        // 设置 __thread修饰的currentthread::线程ID，线程名字长度
        if (t_cachedTid == 0){
            t_cachedTid = detail::gettid();
        }
        return Kukai::CurrentThread::t_cachedTid;
    }

    // thread类中的static变量，表示当前进程创建的线程数目
    AtomicInt32 Thread::numCreated_;

    /*
     * 构造函数，初始化线程信息，是否启动，回收，线程ID，进程ID，执行函数，线程名字，倒计时计数
     * */
    Thread::Thread(ThreadFunc func, const std::string &name)
        : started_(false),
          joined_(false),
          pthreadId_(0),
          tid_(0),
          func_(std::move(func)),
          name_(name){
          numCreated_.increment();
    }

    /*
     * 析构时把线程detach，不用父进程回收子进程
     * */
    Thread::~Thread() {
        if (started_ && !joined_){
            pthread_detach(pthreadId_);
        }
    }

    /*
     * 启动，利用pthread_create()创建一个线程去执行detail::startThread函数
     * 把ThreadData指针作为参数传入，在startThread函数中执行ThreadData::runInThread
     * 完成具体函数的线程函数调用
     * */
    void Thread::start() {
        assert(!started_);
        started_ = true;

        detail::ThreadData *data = new detail::ThreadData(func_, name_, &tid_);
        if (pthread_create(&pthreadId_, nullptr, &detail::startThread, data)){
            // 线程创建失败
            started_ = false;
            delete data;
            abort();
        }
    }

    /*
     * 回收子进程
     * */
    void Thread::join() {
        assert(started_);
        assert(!joined_);
        joined_ = true;
        pthread_join(pthreadId_, nullptr);
    }
}   //namespace Kukai
