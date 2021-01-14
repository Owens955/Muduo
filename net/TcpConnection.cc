//
// Created by Kukai on 2020/12/29.
//

#include "TcpConnection.h"

#include "../base/Log.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"
#include "SocketsOps.h"

#include <functional>

#include <cerrno>
#include <cstdio>

using namespace Kukai;

TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string &nameArg,
                             int sockfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
         : loop_(loop),
           name_(nameArg),
           state_(kConnecting),
           socket_(new Socket(sockfd)),
           channel_(new Channel(loop, sockfd)),
           localAddr_(localAddr),
           peerAddr_(peerAddr){
    LOG("TcpConnection::ctor[" << name_ << "] at " << this
                               << " fd = " << sockfd);
    channel_->setReadCallback(
            std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(
            std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(
            std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(
            std::bind(&TcpConnection::handleError, this));
}

TcpConnection::~TcpConnection() {
    LOG("TcpConnect::dtor[" << name_ << "] at " << this
                            << " fd = " << channel_->fd());
}

void TcpConnection::send(const std::string &message) {
    if (state_ == kConnected){
        if (loop_->isInLoopThread()){
            sendInLoop(message);
        } else {
            loop_->runInLoop(
                    std::bind(&TcpConnection::sendInLoop, this, message));
        }
    }
}

void TcpConnection::sendInLoop(const std::string &message) {
    loop_->assertInLoopThread();
    ssize_t nwrote = 0;
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        nwrote = ::write(channel_->fd(), message.data(), message.size());
        if (nwrote >= 0) {
            if (static_cast<size_t>(nwrote) < message.size()) {
                LOG("I am going to write more data");
            } else if (writeCompleteCallback_){
                loop_->queueInLoop(
                        std::bind(&TcpConnection::writeCompleteCallback_, shared_from_this()));
            }
        } else {
            nwrote = 0;
            if (errno != EWOULDBLOCK) {
                LOG("TcpConnection::sendInLoop");
            }
        }
    }
    assert(nwrote >= 0);
    if (static_cast<size_t>(nwrote) < message.size()){
        outputBuffer_.append(message.data()+nwrote, message.size()-nwrote);
        if (!channel_->isWriting()){
            channel_->enableWriting();
        }
    }
}

void TcpConnection::shutdown() {
    if (state_ == kConnected){
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop() {
    loop_->assertInLoopThread();
    if (!channel_->isWriting()){
        socket_->shutdownWrite();
    }
}

void TcpConnection::setTcpNoDelay(bool on) {
    socket_->setTcpNoDelay(on);
}

void TcpConnection::connectEstablished() {
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);
    setState(kConnected);
    channel_->enableReading();
    if(connectionCallback_)
        connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed() {
    loop_->assertInLoopThread();
    assert(state_ == kConnected || state_ == kDisconnecting);
    setState(kDisconnected);
    channel_->disableAll();
    if(connectionCallback_)
        connectionCallback_(shared_from_this());

    loop_->removeChannel(channel_.get());
}

void TcpConnection::handleRead(Timestamp receiveTime) {
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0){
        if(messageCallback_)
            messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    } else if ( n == 0){
        handleClose();
    } else {
        errno = savedErrno;
        LOG("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite() {
    loop_->assertInLoopThread();
    if (channel_->isWriting()){
        ssize_t n = ::write(channel_->fd(),
                            outputBuffer_.peek(),
                            outputBuffer_.readableBytes());
        if (n > 0){
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0){
                channel_->disableWriting();
                if (writeCompleteCallback_){
                    loop_->queueInLoop(
                            std::bind(&TcpConnection::writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting){
                    shutdownInLoop();
                }
            } else {
                LOG("I am going to write more data");
            }
        } else {
            LOG("TcpConnection::handleWrite");
        }
    } else {
        LOG("Connection is down, no more writing");
    }
}

void TcpConnection::handleClose() {
    loop_->assertInLoopThread();
    LOG("TcpConnection::handleClose state = " << state_);
    assert(state_ == kConnected || state_ == kDisconnecting);
    channel_->disableAll();
    if(closeCallback_)
        closeCallback_(shared_from_this());
}

void TcpConnection::handleError() {
    int err = sockets::getSocketError(channel_->fd());
    LOG("TcpConnection::handleError [" << name_
        << "] - SO_ERROR = "
        << err << " "
        << strerror(err));
}

