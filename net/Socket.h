//
// Created by Kukai on 2020/12/25.
//

#ifndef MUDUO_NET_SOCKET_H
#define MUDUO_NET_SOCKET_H

#include "../boost/noncopyable.h"

namespace Kukai{
    class InetAddress;

    class Socket : boost::noncopyable {
    public:
        explicit Socket(int sockfd)
            : sockfd_(sockfd)
            {   }

        ~Socket();

        int fd() const  { return sockfd_; }

        void bindAddress(const InetAddress &addr);

        void listen();

        int accept(InetAddress *peeraddr);

        void setReuseAddr(bool on);

        void shutdownWrite();

        void setTcpNoDelay(bool on);

    private:
        const int sockfd_;
    };
}

#endif //MUDUO_NET_SOCKET_H
