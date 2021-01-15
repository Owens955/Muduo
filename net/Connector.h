//
// Created by Kukai on 2021/1/5.
//

#ifndef MUDUO_NET_CONNECTOR_H
#define MUDUO_NET_CONNECTOR_H

#include "InetAddress.h"
#include "../boost/noncopyable.h"
#include "Timerld.h"

#include <memory>
#include <functional>

namespace Kukai{

    class EventLoop;
    class Channel;

    class Connector : boost::noncopyable {
    public:
        typedef std::function<void (int sockfd)> NewConnectionCallback;

        Connector(EventLoop *loop, const InetAddress &serverAddr);
        ~Connector();

        void setNewConnectionCallback(const NewConnectionCallback &cb)
        { newConnectionCallback_ = cb; }

        void start();
        void restart();
        void stop();

        const InetAddress &serverAddress() const { return serverAddr_; }

    private:
        enum States{ kDisconnected, kConnecting, kConnected };
        static const int kMaxRetryDelayMs = 30*1000;
        static const int kInitRetryDelayMs = 500;

        void setState(States s) { state_ = s; }
        void startInLoop();
        void connect();
        void connecting(int sockfd);
        void handleWrite();
        void handleError();
        void retry(int sockfd);
        int removeAndResetChannel();
        void resetChannel();

        EventLoop *loop_;
        InetAddress serverAddr_;
        bool connect_;
        States state_;
        std::unique_ptr<Channel> channel_;
        NewConnectionCallback newConnectionCallback_;
        int retryDelayMs_;
        Timerld timerld_;
    };
    typedef std::shared_ptr<Connector> ConnectorPtr;
}

#endif //MUDUO_NET_CONNECTOR_H
