//
// Created by Kukai on 2021/1/5.
//

#include "TcpClient.h"

#include "EventLoop.h"
#include "Connector.h"
#include "../base/Log.h"
#include "SocketsOps.h"

#include <functional>
#include <stdio.h>

using namespace Kukai;

namespace Kukai{
    namespace detail{
        void removeConnection(EventLoop *loop, const TcpConnectionPtr &conn){
            loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
        }

        void removeConnector(const ConnectorPtr &connector){

        }
    }
}

TcpClient::TcpClient(EventLoop *loop, const InetAddress &serverAddr)
    : loop_(loop),
      connector_(new Connector(loop, serverAddr)),
      retry_(false),
      connect_(true),
      nextConnld_(1)
      {
         connector_->setNewConnectionCallback(
                 std::bind(&TcpClient::newConnection, this, std::placeholders::_1));
         LOG("TcpClient::TcpClient[" << this
             << "] - connector " << connector_.get());
      }

TcpClient::~TcpClient() {
    LOG("TcpClient::~TcpClient[" << this
        << "] - connector " << connector_.get());
    TcpConnectionPtr conn;
    {
        MutexLockGuard lock(mutex_);
        conn = connection_;
    }
    if (conn){
        CloseCallback cb = std::bind(&detail::removeConnection, loop_, std::placeholders::_1);
        loop_->runInLoop(
                std::bind(&TcpConnection::setCloseCallback, conn, cb));
    } else {
        connector_->stop();
        loop_->runAfter(1, std::bind(&detail::removeConnector, connector_));
    }
}

void TcpClient::connect(){
    LOG("TcpClient::connect[" << this << "] - connecting to "
        << connector_->serverAddress().toHostPort());
    connect_ = true;
    connector_->start();
}

void TcpClient::disconnect() {
    connect_ = false;

    {
        MutexLockGuard lock(mutex_);
        if (connection_){
            connection_->shutdown();
        }
    }
}

void TcpClient::stop() {
    connect_ = false;
    connector_->stop();
}

void TcpClient::newConnection(int sockfd) {
    loop_->assertInLoopThread();
    InetAddress peerAddr(sockets::getLocalAddr(sockfd));
    char buf[32];
    snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toHostPort().c_str(), nextConnld_);
    ++nextConnld_;
    std::string connName = buf;

    InetAddress localAddr(sockets::getLocalAddr(sockfd));
    TcpConnectionPtr conn(new TcpConnection(
            loop_,
            connName,
            sockfd,
            localAddr,
            peerAddr));
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
            std::bind(&TcpClient::removeConnection, this, std::placeholders::_1));
    {
        MutexLockGuard lock(mutex_);
        connection_ = conn;
    }
    conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr &conn) {
    loop_->assertInLoopThread();
    assert(loop_ == conn->getLoop());

    {
        MutexLockGuard lock(mutex_);
        assert(connection_ == conn);
        connection_.reset();
    }

    loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    if (retry_ && connect_){
        LOG("TcpClient::connect[" << this << "] - Reconnecting to "
            << connector_->serverAddress().toHostPort());
        connector_->restart();
    }
}
