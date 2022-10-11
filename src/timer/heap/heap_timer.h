//
// Created by nie on 22-9-16.
//

#ifndef NEREIDS_HEAP_TIMER_H
#define NEREIDS_HEAP_TIMER_H


#include <ctime>
#include <netinet/in.h>
#include <memory>
#include <set>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <csignal>
#include <cstring>
#include <cassert>
#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include <unistd.h>
#include "../util_timer.h"


//定时器容器
class heap_timer {
public:
    //构造函数
    heap_timer() {
        heap_.reserve(64);
    }

    //销毁函数
    ~heap_timer() {
        init();
    }

    //将目标定时器timer添加到堆中
    void add_timer(util_timer *timer);


    //调整目标定时器的定时时间
    void adjust_timer(util_timer *timer);

    void doWork(util_timer *timer);

    //SIGALRM信号每次被触发就在其信号处理函数中执行一次tick函数，以处理链表上到期的任务
    void tick();

    void pop_timer();

    int GetNextTick();

    //初始化
    void init();

    //删除目标定时器的定时时间
    void del_timer(util_timer *timer);

private:
    std::vector<util_timer *> heap_;
    std::unordered_map<client_data *, size_t> ref_;

    void del_(size_t idx);

    void shift_up_(size_t i);

    bool shift_down_(size_t idx, size_t n);

    void SwapNode_(size_t i, size_t j);


};


#endif //NEREIDS_HEAP_TIMER_H
