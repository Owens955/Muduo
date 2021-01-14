//
// Created by Kukai on 2020/12/8.
//

#ifndef MUDUO_NET_CHANNEL_H
#define MUDUO_NET_CHANNEL_H

#include "../boost/noncopyable.h"

#include <functional>
#include <memory>
#include "../base/Timestamp.h"

namespace Kukai{

        class EventLoop;

        /*
         * IO事件分发器
         * */
        class Channel : boost::noncopyable {
        public:
            typedef std::function<void()> EventCallback;
            typedef std::function<void(Timestamp)> ReadEventCallback;
            Channel(EventLoop *loop , int fd);
            ~Channel();

            void handleEvent(Timestamp receiveTime);

            /*
             * 设置回调函数，read，write，error Callback函数，在处理event时被调用
             * */
            void setReadCallback(const ReadEventCallback &cb){ readCallback_ = std::move(cb); }
            void setWriteCallback(const EventCallback cb){ writeCallback_ = std::move(cb); }
            void setErrorCallback(const EventCallback cb){ errorCallback_ = std::move(cb); }
            void setCloseCallback(const EventCallback cb){ closeCallback_ = std::move(cb); }

            /*
             * channel所负责IO事件的那个fd
             * */
            int fd() const { return fd_;}
            /*
             * 返回channel所注册的那个网络事件
             * */
            int events() const { return events_;}
            /*
             * 设置网络事件
             * */
            void set_revents(int revt) { revents_ = revt; }
            /*
             * 判断当前channel是否注册了事件
             * */
            bool isNoneEvent() const { return events_ == kNoneEvent; }

            /*
             * 在event上注册读/写事件
             * */
            void enableReading() { events_ |= kReadEvent; update(); }
            void enableWriting() { events_ |= kWriteEvent; update(); }
            /*
             * 在event上取消读/写事件
             * */
             void disableWriting() { events_ &= ~kWriteEvent; update(); }
             void disableAll() { events_ = kNoneEvent; update(); }
             bool isWriting() const { return events_ & kWriteEvent; }
            /*
             * for poller
             * */
            int index() { return index_; }
            void set_index(int idx) { index_ = idx; }

            void removeChannel();

            /*
             * 返回当前channel所在的那个eventloop
             * */
            EventLoop *ownerLoop() { return loop_;}

        private:
            /*
             * 让本channel所属于的那个eventloop回调channel::update()完成channel的更新
             * */
            void update();

            /*
             * 这三个静态常量分别表示：无网络事件，读网络事件，写网络事件
             * */
            static const int kNoneEvent;
            static const int kReadEvent;
            static const int kWriteEvent;

            EventLoop* loop_;       // channel所属的那个eventloop
            const int  fd_;         // 每个channel负责处理一个sockfd上的网络事件
            int        events_;     // channel注册（要监听）的网络事件
            int        revents_;    // poll()返回的网络事件，具体发生的事件
            int        index_;      // 这个channel在poller中pollfds中的序号，默认-1表示不在其中

            bool eventHandling_;

            /*
             * 当发生了读/写/错误网络事件时，下面三个函数会被调用
             * */
            ReadEventCallback readCallback_;
            EventCallback writeCallback_;
            EventCallback closeCallback_;
            EventCallback errorCallback_;

        };
}

#endif //MUDUO_NET_CHANNEL_H
