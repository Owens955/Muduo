//
// Created by Kukai on 2021/1/5.
//

#include "Connector.h"

#include "../base/Log.h"
#include "Channel.h"
#include "EventLoop.h"
#include "SocketsOps.h"

#include <functional>
#include <cerrno>

using namespace Kukai;

const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop *loop, const InetAddress &serverAddr)
    : loop_(loop),
      serverAddr_(serverAddr),
      connect_(false),
      state_(kDisconnected),
      retryDelayMs_(kInitRetryDelayMs)
      {
      LOG("ctor[" << this << "]");
      }

Connector::~Connector(){
    LOG("dtor[" << this << "]");
    loop_->cancel(timerld_);
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
    int ret = sockets::connect(sockfd, serverAddr_.getSockAddrInet());
    int savedErrno = (ret == 0) ? 0 : errno;

    switch (savedErrno)
    {
        case 0:
        case EINPROGRESS:
        case EINTR:
        case EISCONN://连接成功
            connecting(sockfd);
            break;

        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
        case ENETUNREACH:
            retry(sockfd);
            break;

        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
            LOG( "connect error in Connector::startInLoop " << savedErrno);
            sockets::close(sockfd);
            break;

        default:
            LOG("Unexpected error in Connector::startInLoop " << savedErrno);
            sockets::close(sockfd);
            // connectErrorCallback_();
            break;
    }
}

void Connector::restart() {
    loop_->assertInLoopThread();
    setState(kDisconnected);
    retryDelayMs_ = kInitRetryDelayMs;
    connect_ = true;
    startInLoop();
}

void Connector::stop() {
    connect_ = false;
    loop_->cancel(timerld_);
}

void Connector::connecting(int sockfd) {
    assert(kConnecting);
    assert(!channel_);
    channel_.reset(new Channel(loop_, sockfd));
    channel_->setWriteCallback(
            std::bind(&Connector::handleWrite, this));
    channel_->setErrorCallback(
            std::bind(&Connector::handleError, this));
    channel_->enableWriting();
}

int Connector::removeAndResetChannel() {
    channel_->disableAll();
    loop_->removeChannel(channel_.get());
    int sockfd = channel_->fd();
    loop_->queueInLoop(
            std::bind(&Connector::resetChannel, this));
    return sockfd;
}

void Connector::resetChannel() {
    channel_.reset();
}

void Connector::handleError() {
    LOG("Connect::handleError ");
    assert(state_ == kConnecting);

    int sockfd = removeAndResetChannel();
    int err = sockets::getSocketError(sockfd);
    LOG("SO_ERROR = " << err << " " << strerror(err));
    retry(sockfd);
}

void Connector::handleWrite() {
    LOG("Connect::handleWrite " << state_);
    if (state_ == kConnecting){
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        if (err){
            LOG("Connector::handleWrite - Self connect");
            retry(sockfd);
        } else if (sockets::isSelfConnect(sockfd)){
            LOG("Connect::handleWrite - Self connect");
            retry(sockfd);
        } else {
            setState(kConnected);
            if (connect_){
                newConnectionCallback_(sockfd);
            } else {
                sockets::close(sockfd);
            }
        }
    } else {
        assert(state_ == kDisconnected);
    }
}

void Connector::retry(int sockfd) {
    sockets::close(sockfd);
    setState(kDisconnected);
    if (connect_){
        LOG("Connector::retry - Retry connecting to "
                        << serverAddr_.toHostPort() << " in "
                        << retryDelayMs_ << " milliseconds.");
        timerld_ = loop_->runAfter(retryDelayMs_/1000.0,
                                   std::bind(&Connector::startInLoop, this));
        retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
    } else {
        LOG("do not connect");
    }
}