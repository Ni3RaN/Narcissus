//
// Created by nie on 22-10-10.
//

#ifndef NEREIDS_SET_TIMER_H
#define NEREIDS_SET_TIMER_H


#include <netinet/in.h>
#include <ctime>
#include <chrono>
#include <set>
#include "../util_timer.h"

typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point time_point;


class set_timer {
public:
    set_timer() {
        init();
    }

    ~set_timer() {
        init();
    }

    void init();

    void add_timer(util_timer *timer);

    void del_timer(util_timer *timer);

    void adjust_timer(util_timer *timer);

    void tick();

private:
    std::multiset<util_timer *> timer_set; //定时器容器
};


#endif //NEREIDS_SET_TIMER_H
