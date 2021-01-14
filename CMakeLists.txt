# cmake_minimum_required(VERSION <specify CMake version here>)
cmake_minimum_required(VERSION 2.8.12)
project(Muduo)

set(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_FLAGS -pthread)
# TODO 这里去看一下cmakelist怎么写的

add_executable(Muduo main.cpp net/EventLoop.cc base/Thread.cc base/CurrentThread.cc net/Channel.cc net/Channel.h net/Poller.cc net/Poller.h base/Timestamp.cc base/Timestamp.h net/Callbacks.h net/Timer.cc net/Timer.h net/Timerld.h net/TimerQueue.cc net/TimerQueue.h net/EventLoopThread.cc net/EventLoopThread.h base/Mutex.h base/Condition.cc base/Condition.h base/Log.h net/Socket.cc net/Socket.h net/SocketsOps.cc net/SocketsOps.h net/InetAddress.cc net/InetAddress.h net/Acceptor.cc net/Acceptor.h net/TcpConnection.cc net/TcpConnection.h net/TcpServer.cc net/TcpServer.h net/Buffer.cc net/Buffer.h net/TcpClient.cc net/TcpClient.h net/Connector.cc net/Connector.h net/EventLoopThreadPool.cc net/EventLoopThreadPool.h)

#include_directories(SYSTEM "/foo/bar")