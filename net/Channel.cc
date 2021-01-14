//
// Created by Kukai on 2020/12/8.
//

#include "Channel.h"
#include "EventLoop.h"
#include "../base/Log.h"

#include <sstream>
#include <iostream>
#include <poll.h>

using namespace Kukai;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop *loop, int fdArg)
        : loop_(loop),
          fd_(fdArg),
          events_(0),
          revents_(0),
          index_(-1),
          eventHandling_(false)
{  }

Channel::~Channel() {
    assert(!eventHandling_);
}

void Channel::update() {
    loop_->updateChannel(this);
}

/*
 * 此时eventloop::loop()中poll函数返回，说明有网络事件发生了
 * 针对网络
 * 络事件类型进行相应处理
 * */
void Channel::handleEvent(Timestamp receiveTime) {
    eventHandling_ = true;
    // 处理网络事件 POLLNVAL
    if (revents_ & POLLNVAL){
        LOG("Channel::handle_event() POLLNVAL");
    }
    // 处理网络事件 关闭连接
    if ((revents_ & POLLHUP) && !(revents_ & POLLIN)){
        LOG("Channel::handle_event() POLLHUP");
        if (closeCallback_) closeCallback_();
    }
    // 处理网络事件 错误
    if (revents_ &(POLLNVAL | POLLERR)){
        if (errorCallback_) errorCallback_();
    }
    // 处理网络事件 read
    if (revents_ &(POLLIN | POLLPRI | POLLRDHUP)){
        if (readCallback_) readCallback_(receiveTime);
    }
    // 处理网络事件 write
    if (revents_ &POLLOUT){
        if (writeCallback_) writeCallback_();
    }
    eventHandling_ = false;
}

void Channel::removeChannel() {
    loop_->removeChannel(this);
}
