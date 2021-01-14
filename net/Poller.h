//
// Created by Kukai on 2020/12/8.
//

#ifndef MUDUO_NET_POLLER_H
#define MUDUO_NET_POLLER_H

#include <map>
#include <sys/poll.h>
#include <vector>

#include "../base/Timestamp.h"
#include "EventLoop.h"

namespace Kukai{
        class Channel;

        class Poller : boost::noncopyable{
        public:
            typedef std::vector<Channel*> ChannelList;
            /*
             * 构造函数，指定poller所属的那个eventloop对象
             * */
            Poller(EventLoop *loop);
            ~Poller() = default;

            /*
             * 作为poller函数的核心，eventloop在loop()中调用poll()函数获得当前活动的IO事件
             * 然后填充eventloop生成所有channel到channelList中，保存所有的channel信息
             * */
            Timestamp poll(int timeoutMs, ChannelList* activeChannels);

            /*
             * 负责更新把channel更新到pollfds
             * */
            void updateChannel(Channel *channel);
            //virtual void removeChannell(Channel *channel) = 0;
            void removeChannel(Channel *channel);
            /*
             * 保证eventloop所在线程为当前线程
             * */
            void assertInLoopThread() { ownerLoop_ -> assertInLoopThread();}

        private:
            void fillActiveChannels(int numEvents,
                                    ChannelList* activeChannels) const;


            typedef std::vector<struct pollfd> PollFdList;
            typedef std::map<int, Channel*> ChannelMap;

            EventLoop *ownerLoop_;  // poller所属于的那个eventloop对象
            PollFdList pollfds_;    // pollfd集合，用户保存所有客户机套接字信息
            ChannelMap channels_;   // poller所属于的那个eventloop对象
        };
}

#endif //MUDUO_NET_POLLER_H
