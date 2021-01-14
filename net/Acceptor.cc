//
// Created by Kukai on 2020/12/29.
//

#include "Acceptor.h"

#include "../base/Log.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "SocketsOps.h"

#include <functional>

using namespace Kukai;

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr)
    : loop_(loop),
      acceptSocket_(sockets::createNonblockingOrDie()),
      acceptChannel_(loop, acceptSocket_.fd()),
      listenning_(false){
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.bindAddress(listenAddr);
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

void Acceptor::listen() {
    loop_->assertInLoopThread();
    listenning_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

void Acceptor::handleRead() {
    loop_->assertInLoopThread();
    InetAddress peerAddr(0);

    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0){
        if (newConnectionCallback_){
            newConnectionCallback_(connfd, peerAddr);
        } else {
            sockets::close(connfd);
        }
    }
}