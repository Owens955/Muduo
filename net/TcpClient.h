//
// Created by Kukai on 2021/1/5.
//

#ifndef MUDUO_NET_TCPCLIENT_H
#define MUDUO_NET_TCPCLIENT_H

#include <cstring>
#include <memory>
#include "../base/Mutex.h"
#include "Callbacks.h"
#include "TcpConnection.h"

namespace Kukai{

    class Connector;
    typedef std::shared_ptr<Connector> ConnectorPtr;

    class TcpClient : boost::noncopyable{
    public:
        TcpClient(EventLoop *loop,const InetAddress &serverAddr);
        ~TcpClient();

    void connect();
    void disconnect();
    void stop();

    TcpConnectionPtr connection() const{
        MutexLockGuard lock(mutex_);
        return connection_;
    }

    bool retry() const;
    void enableRetry() { retry_ = true; }

    void setNewConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }

    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }

    private:

        void newConnection(int sockfd);
        void removeConnection(const TcpConnectionPtr &conn);

        EventLoop *loop_;
        ConnectorPtr connector_;
        ConnectionCallback connectionCallback_;
        MessageCallback messageCallback_;
        WriteCompleteCallback writeCompleteCallback_;
        bool retry_;
        bool connect_;
        int nextConnld_;
        mutable MutexLock mutex_;
        TcpConnectionPtr connection_;
    };
}

#endif //MUDUO_NET_TCPCLIENT_H
