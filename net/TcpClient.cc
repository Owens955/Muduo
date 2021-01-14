//
// Created by Kukai on 2021/1/5.
//

#include "TcpClient.h"
#include "EventLoop.h"
#include "Connector.h"
#include "../base/Log.h"
#include "SocketsOps.h"

#include <functional>

using namespace Kukai;

TcpClient::TcpClient(EventLoop *loop, const InetAddress &connectAddr, const std::string & name)
    : loop_(loop),
      name_(name),
      connection_(nullptr),
      connector_(new Connector(loop, connectAddr))
      {
         connector_->setConnectionCallback(
                 std::bind(&TcpClient::newConnection, this, std::placeholders::_1, std::placeholders::_2));
      }

TcpClient::~TcpClient() {
}

void TcpClient::newConnection(int sockfd, InetAddress &connectAddr) {
    loop_->assertInLoopThread();
    InetAddress localAddr(sockets::getLocalAddr(sockfd));
    std::string conName = name_;
    connection_.reset(new TcpConnection(
            loop_,
            conName,
            sockfd,
            localAddr,
            connectAddr));
    connection_->setConnectionCallback(connectionCallback_);
    connection_->setMessageCallback(messageCallback_);
    connection_->setWriteCompleteCallback(writeCompleteCallback_);
    /*conn->setCloseCallback(
            std::bind(&TcpClient::))*/
    connection_->connectEstablished();
}

void TcpClient::Connect() {
    connector_->start();
}