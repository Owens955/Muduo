#include "net/TcpServer.h"
#include "net/TcpClient.h"
#include "net/EventLoop.h"
#include "base/Log.h"
#include <cstdio>
#include <iostream>
#include <vector>
std::string message;

void onConnection(const Kukai::TcpConnectionPtr& conn)
{
    if (conn->connected())
    {
        printf("onConnection(): new connection [%s] from %s\n",
               conn->name().c_str(),
               conn->peerAddress().toHostPort().c_str());
    }
    else
    {
        printf("onConnection(): connection [%s] is down\n",
               conn->name().c_str());
    }
}

void onWriteComplete(const Kukai::TcpConnectionPtr& conn)
{
    std::cin >> message;
    conn->send(message);
    message.clear();
}

void onMessage(const Kukai::TcpConnectionPtr& conn,
               Kukai::Buffer* buf,
               Kukai::Timestamp receiveTime)
{
    LOG("onMessage(): received " << buf->readableBytes() <<
                 "d bytes from connection [" << conn->name().c_str() <<
                 "] at " << receiveTime.toFormattedString().c_str() <<
                 "\n " << buf->retrieveAsString());
    buf->retrieveAll();
}

void OnConnection(const Kukai::TcpConnectionPtr &conn){
    if (conn->connected()){
        LOG("connect successs");
    } else {
        printf("connect fail\n");
    }
}

void OnMassage(const Kukai::TcpConnectionPtr &conn , Kukai::Buffer *buf, Kukai::Timestamp time){
    std::string msg(buf->retrieveAsString());
    printf("onMassage() : recv a message : %s\n", msg.c_str());

}

void handleWrite(const Kukai::TcpConnectionPtr &conn ){
    char buf[1024] = {0};
    fgets(buf, 1024 , stdin);
    buf[strlen(buf) - 1] = '\0';
    conn->send(buf);
}

void randomString(std::string & str,ssize_t len){
    str.resize(len);
    srand(time(nullptr));
    for(auto it = str.begin(); it != str.end(); ++it){
        auto randnum = rand();
        *it =randnum %2 == 1? rand() % 9 + '0' : randnum % 26 + 'a';

    }
}
void OnConnectionTest(const Kukai::TcpConnectionPtr &conn){

    std::vector<std::string> vec(100);
    if (conn->connected()){
        LOG("connect successs");
        for(ssize_t  i = 0; i < vec.size(); ++i){
            randomString(vec[i],1024);
            conn->send(vec[i]);
        }
    } else {
        printf("connect fail\n");
    }

}

int main(int argc, char* argv[])
{
    printf("main(): pid = %d\n", getpid());


    if (argc < 2){
    /*服务端*/
    Kukai::InetAddress listenAddr(9981);
    Kukai::EventLoop loop;
    Kukai::TcpServer server(&loop, listenAddr);
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    server.setWriteCompleteCallback(onWriteComplete);
    server.start();

    loop.loop();
    } else {
    /*客户端*/

    Kukai::InetAddress connectAddr("192.168.1.101", 9981);
    Kukai::EventLoop loop;

    Kukai::TcpClient client(&loop, connectAddr,"client");

    client.setConnectionCallback(OnConnection);
    client.setMessageCallback(OnMassage);
    client.setWriteCompleteCallback(handleWrite);
    client.Connect();
    loop.loop();
    }
}
