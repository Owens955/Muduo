//
// Created by Kukai on 2020/12/29.
//

#ifndef MUDUO_NET_ACCEPTOR_H
#define MUDUO_NET_ACCEPTOR_H

#include <functional>
#include "../boost/noncopyable.h"
#include "Channel.h"
#include "Socket.h"

namespace Kukai{

    class EventLoop;
    class InetAddress;

    class Acceptor : boost::noncopyable {
    public:
        typedef std::function<void (int sockfd, const InetAddress&)> NewConnectionCallback;

        Acceptor(EventLoop *loop, const InetAddress &listenAddr);

        void setNewConnectionCallback(const NewConnectionCallback& cb) { newConnectionCallback_ = cb; }

        bool listening() const { return listenning_; }
        void listen();

    private:

        void handleRead();
        EventLoop *loop_;
        Socket acceptSocket_;
        Channel acceptChannel_;
        NewConnectionCallback newConnectionCallback_;
        bool listenning_;

    };
}

#endif //MUDUO_NET_ACCEPTOR_H
