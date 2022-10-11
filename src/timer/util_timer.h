//
// Created by nie on 22-10-11.
//

#ifndef NEREIDS_UTIL_TIMER_H
#define NEREIDS_UTIL_TIMER_H

#include <netinet/in.h>
#include <chrono>

typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point time_point;


//定时器类，提前
class util_timer;

//用户数据结构
struct client_data {
    sockaddr_in address;
    int sockfd;
    util_timer *timer;
};

//定时器类
class util_timer {
public:
    //定时器生效的绝对时间
    time_point expire;

    //定时器回调函数
    void (*cb_func)(client_data *){};

    //回调函数处理的客户数据，由定时器的执行者传递给回调函数
    client_data *user_data{};

    //重载运算符，用于重置定时器
    bool operator<(const util_timer &rhs) const {
        return expire < rhs.expire;
    }
};

#endif //NEREIDS_UTIL_TIMER_H
