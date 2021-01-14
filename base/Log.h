//
// Created by 空海lro on 2020/12/25.
//

#ifndef MUDUO_BASE_LOG_H
#define MUDUO_BASE_LOG_H

#include <cstdio>
#include <string>
#include <iostream>
#include <sstream>

#define LOG(message)   {std::stringstream stream; \
                        stream << message; \
                        std::cout << __FILE__ << ":" <<__LINE__ << "  " << stream.str() << std::endl;}

#endif //MUDUO_BASE_LOG_H
