//
// Created by Kukai on 2020/12/8.
//

#include "Poller.h"
#include "../base/Log.h"
#include "Channel.h"

#include <cassert>
#include <iostream>

using namespace Kukai;
Poller::Poller(EventLoop *loop)
    : ownerLoop_(loop){}

/*
 * 调用::poll()获取发生了网络事件的套接字，并为每个网络事件分配一个channel用于处理
 * 函数返回当前::poll()返回的时间戳
 * */
Timestamp Poller::poll(int timeoutMs, ChannelList *activeChannels) {
    int numEvents = ::poll(pollfds_.data(), pollfds_.size(), timeoutMs);

    Timestamp now(Timestamp::now());

    // 此时poll返回，numEvents个套接字上发生了网络事件
    if (numEvents > 0){
        LOG("" << numEvents << " event happended");
        // 在activeChannels中添加numThreads个channel，每个事件分配一个channel
        fillActiveChannels(numEvents, activeChannels);
    } else if (numEvents == 0){
        LOG("nothing happended");
    } else {
        LOG("Poller::poll()");
    }
    return now;
}

/*
 * 遍历pollfds，找出当前活动（发生网络事件）的套接字，把它相对应的channel加入到activeChannels
 * 这个时候poll和epoll的区别就体现出来了
 * poll需要轮寻pollfd数组，而epoll直接返回发生了网络事件的epollfd数组，不需要轮寻
 * */
void Poller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const {
    // 轮寻pollfd数组，找到发生网络事件的套接字，为其分配channel加入到activeChannel
    for (auto pfd = pollfds_.begin();
         pfd != pollfds_.end() && numEvents > 0 ; ++pfd) {
        if (pfd->revents > 0){
            // 找到这个套接字的channel
            --numEvents;
            auto ch = channels_.find(pfd->fd);
            assert(ch != channels_.end());
            Channel *channel = ch->second;
            assert(channel->fd() == pfd->fd);
            // 当前套接字是否发生了网络事件，是的话就把他对应的channel填充到activesChannel中
            channel->set_revents(pfd->revents);
            activeChannels->push_back(channel);
        }
    }
}

/*
 * 主要功能是负责维护和更新pollfd数组
 * 传入一个channel进来，我们得到ch->index()判断它在pollfds中的位置，如果不存在
 * 需要在pollfds中新加一个pollfd
 * 如果存在直接更新即可
 * */
void Poller::updateChannel(Channel *channel) {
    // 保证eventloop所在的线程就是其所属的线程
    assertInLoopThread();
    LOG("fd = " << channel->fd() << " events = " << channel->events());
    // 更新pollfds数组
    // 先判断channel这个channel是否已经在channels中，如果不在 index()<0
    if (channel->index() < 0){
        // channel->index()==-1，说明这个channel对应的套接字不在pollfds中添加
        assert(channels_.find(channel->fd()) == channels_.end());
        // 新建一个pollfd
        struct pollfd pfd{};
        pfd.fd = channel->fd();
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        // 加入到pollfds中
        pollfds_.push_back(pfd);
        int idx = static_cast<int>(pollfds_.size())-1;
        channel->set_index(idx);
        channels_[pfd.fd] = channel;
    } else {
        // channel对应的套接字在pollfds中修改
        assert(channels_.find(channel->fd()) != channels_.end());
        assert(channels_[channel->fd()] == channel);
        int idx = channel->index();
        assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
        // 修改pollfds中的这个pollfd
        struct pollfd &pfd = pollfds_[idx];
        assert(pfd.fd == channel->fd() || pfd.fd == -channel->fd()-1);
        pfd.events = static_cast<short>(channel->events());
        pfd.events = 0;
        if (channel -> isNoneEvent()){
            pfd.fd = -channel->fd()-1;
        }
    }
}

void Poller::removeChannel(Channel *channel) {
    assertInLoopThread();
    LOG("fd = " << channel->fd());
    assert(channels_.find(channel->fd()) != channels_.end());
    assert(channels_[channel->fd()] == channel);
    assert(channel->isNoneEvent());
    int idx = channel->index();
    assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
    const struct pollfd & pfd = pollfds_[idx]; (void)pfd;
    assert(pfd.fd == -channel->fd()-1 && pfd.events == channel->events());
    size_t n = channels_.erase(channel->fd());
    size_t(n == 1); (void)n;
    if (static_cast<size_t>(idx) == pollfds_.size() - 1){
        pollfds_.pop_back();
    } else {
        int channelAtEnd = pollfds_.back().fd;
        iter_swap(pollfds_.begin()+idx,  pollfds_.end() - 1);
        if (channelAtEnd < 0){
            channelAtEnd = -channelAtEnd - 1;
        }
        channels_[channelAtEnd]->set_index(idx);
        pollfds_.pop_back();
    }
}