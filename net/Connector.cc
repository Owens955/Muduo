//
// Created by Kukai on 2021/1/5.
//

#include "Connector.h"

#include "../base/Log.h"
#include "Channel.h"
#include "EventLoop.h"
#include "SocketsOps.h"

#include <functional>
#include <errno>

using namespace Kukai;

const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop *loop, const InetAddress &serverAddr)
    : loop_(loop),
      serverAddr_(serverAddr),
      connect_(false),
      state_(kDisconnected),
      retryDelayMs_(kInitRetryDelayMs){

      LOG("ctor[" << this << "]");
}
Connector::~Connector(){
    LOG("dtor[" << this << "]");
    loop_->cancel(timeld_);
    assert(!channel_);
}

void Connector::start() {
    connect_ = true;
    loop_->runInLoop(std::bind(&Connector::startInLoop, this));
}

void Connector::startInLoop(){
    loop_->assertInLoopThread();
    if (connect_){
        connect();
    } else {
        LOG("don‘t connect");
    }
}

void Connector::connect() {
    int sockfd = sockets::createNonblockingOrDie();
    int ret = ::connect(sockfd,reinterpret_cast<const struct sockaddr * >(&connectAddr_.getSockAddrInet()), sizeof(connectAddr_.getSockAddrInet()));
    if (ret < 0){
        LOG("connect fail" << strerror(errno) << errno);
    }

    int savedErrno = (ret == 0) ? 0 : errno;

    switch (savedErrno)
    {
        case 0:
        case EINPROGRESS:
        case EINTR:
        case EISCONN://连接成功
            connecting(sockfd);
            break;
    }
}

void Connector::connecting(int sockfd) {
    assert(!channel_);
    channel_.reset(new Channel(loop_, sockfd));
    channel_->setWriteCallback(
            std::bind(&Connector::handleWrite, this));
    channel_->setErrorCallback(
            std::bind(&Connector::handleError, this));
    channel_->enableWriting();
}

void Connector::handleError() {
    LOG("Connect::handleError");
}

void Connector::handleWrite() {
    LOG("Connect::handleWrite");
    int sockfd = channel_->fd();
    if (connect_){
        channel_->disableAll();
        channel_->removeChannel();
        newConnectionCallback_(sockfd, (InetAddress &) serverAddr_);
    } else {
        sockets::close(sockfd);
    }
}