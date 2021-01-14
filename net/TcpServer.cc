//
// Created by Kukai on 2021/1/4.
//

#include "TcpServer.h"

#include "../base/Log.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "SocketsOps.h"

#include <memory>
#include <functional>
#include <cstdio>

using namespace Kukai;

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr)
    : loop_(loop),
      name_(listenAddr.toHostPort()),
      acceptor_(new Acceptor(loop, listenAddr)),
      threadPool_(new EventLoopThreadPool(loop)),
      started_(false),
      nextConnld_(1)
{
    acceptor_->setNewConnectionCallback(
            std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer() {}

void TcpServer::setThreadNum(int numThreads) {
    assert(0 <= numThreads);
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start() {
    if (!started_){
        started_ = true;
        threadPool_->start();
    }

    if (!acceptor_->listening()){
        loop_->runInLoop(
                std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr) {
    loop_->assertInLoopThread();
    char buf[32];
    snprintf(buf, sizeof buf, "#%d", nextConnld_);
    ++nextConnld_;
    std::string connName = name_ + buf;

    LOG("TcpServer::newConnection ["
        << name_ << "] - new connection ["
        << connName << "] from " << peerAddr.toHostPort());
    InetAddress localAddr(sockets::getLocalAddr(sockfd));
    EventLoop *ioLoop = threadPool_->getNextLoop();
    TcpConnectionPtr conn(
            new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
    connections_[connName] = conn;
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
            std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
    ioLoop->runInLoop(
            std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn) {
    loop_->runInLoop(
            std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn) {
    loop_->assertInLoopThread();
    LOG("TcpServer::removeConnectionInLoop [" << name_
                 << "] - connection " << conn->name());
    size_t n = connections_.erase(conn->name());
    assert(n == 1); (void)n;
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn));

}
