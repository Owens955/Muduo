//
// Created by Kukai on 2021/1/5.
//

#ifndef MUDUO_NET_TCPCLIENT_H
#define MUDUO_NET_TCPCLIENT_H

#include <cstring>
#include "Callbacks.h"
#include "TcpConnection.h"

namespace Kukai{

    class Connector;
    class EventLoop;

    class TcpClient : boost::noncopyable{
    public:
        TcpClient(EventLoop *loop,const InetAddress &connectAddr,const std::string & name);
        ~TcpClient();

    void Connect();

    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }

    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }

    private:

        void newConnection(int sockfd, InetAddress &connectAddr);

        EventLoop *loop_;
        const std::string name_;
        std::unique_ptr<Connector> connector_;
        TcpConnectionPtr connection_;
        ConnectionCallback connectionCallback_;
        MessageCallback messageCallback_;
        WriteCompleteCallback writeCompleteCallback_;

    };
}

#endif //MUDUO_NET_TCPCLIENT_H
