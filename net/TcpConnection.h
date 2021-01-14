//
// Created by Kukai on 2020/12/29.
//

#ifndef MUDUO_NET_TCPCONNECTION_H
#define MUDUO_NET_TCPCONNECTION_H

#include "Callbacks.h"
#include "InetAddress.h"
#include "Buffer.h"

#include "../boost/noncopyable.h"
#include <functional>
#include <memory>

namespace Kukai{

    class Channel;
    class EventLoop;
    class Socket;

    class TcpConnection : boost::noncopyable,
                          public std::enable_shared_from_this<TcpConnection>{
    public:
        TcpConnection(EventLoop *loop,
                      const std::string &name,
                      int sockfd,
                      const InetAddress &localAddr,
                      const InetAddress &peerAddr);
        ~TcpConnection();

        EventLoop *getLoop() const { return loop_; }
        const std::string &name() const { return name_; }
        const InetAddress &localAddress() { return localAddr_; }
        const InetAddress &peerAddress() { return peerAddr_; }
        bool connected() const { return state_ == kConnected; }

        void send(const std::string &message);

        void shutdown();
        void setTcpNoDelay(bool on);

        void setConnectionCallback(const ConnectionCallback &cb){
            connectionCallback_ = cb;
        }

        void setMessageCallback(const MessageCallback &cb){
            messageCallback_ = cb;
        }

        void setWriteCompleteCallback(const WriteCompleteCallback &cb){
            writeCompleteCallback_ = cb;
        }

        void setCloseCallback(const CloseCallback &cb)
        { closeCallback_ = cb; }

        void connectEstablished();

        void connectDestroyed();

    private:
        enum StateE { kConnecting, kConnected, kDisconnecting, kDisconnected};

        void setState(StateE s) { state_ = s; }
        void handleRead(Timestamp receiveTime);
        void handleWrite();
        void handleClose();
        void handleError();
        void sendInLoop(const std::string &message);
        void shutdownInLoop();

        EventLoop *loop_;
        std::string name_;
        StateE state_;

        std::unique_ptr<Socket> socket_;
        std::unique_ptr<Channel> channel_;
        InetAddress localAddr_;
        InetAddress peerAddr_;
        ConnectionCallback connectionCallback_;
        MessageCallback messageCallback_;
        WriteCompleteCallback writeCompleteCallback_;
        CloseCallback closeCallback_;
        Buffer inputBuffer_;
        Buffer outputBuffer_;
    };

    typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
}

#endif //MUDUO_NET_TCPCONNECTION_H
