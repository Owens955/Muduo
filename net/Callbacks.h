//
// Created by Kukai on 2020/12/14.
//

#ifndef MUDUO_NET_CALLBACKS_H
#define MUDUO_NET_CALLBACKS_H

#include <functional>
#include <memory>

#include "../base/Timestamp.h"

namespace Kukai{

    //All client visible callbacks go here.
    class Buffer;
    class TcpConnection;
    typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

    typedef std::function<void()> TimerCallback;
    typedef std::function<void (const TcpConnectionPtr&)> ConnectionCallback;
    typedef std::function<void (const TcpConnectionPtr&,
                                Buffer *buf,
                                Timestamp)> MessageCallback;
    typedef std::function<void (const TcpConnectionPtr&)> WriteCompleteCallback;
    typedef std::function<void (const TcpConnectionPtr&)> CloseCallback;
}

#endif //MUDUO_NET_CALLBACKS_H
