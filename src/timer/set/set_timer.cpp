//
// Created by nie on 22-10-10.
//

#include <cassert>
#include "set_timer.h"
#include "../../webserver.h"


void set_timer::init() {
    timer_set.clear();
}

void set_timer::add_timer(util_timer *timer) {
    assert(timer != nullptr);
    timer_set.insert(timer);
}

void set_timer::adjust_timer(util_timer *timer) {
    assert(timer != nullptr);
    timer_set.erase(timer);
    auto cur = Clock::now();
    timer->expire = cur + MS(3 * TIMESLOT);
    timer_set.insert(timer);
}

void set_timer::del_timer(util_timer *timer) {
    assert(timer != nullptr);
    timer_set.erase(timer);
}

void set_timer::tick() {
    if (timer_set.empty()) {
        return;
    }
    for (auto it: timer_set) {
        if (std::chrono::duration_cast<MS>(it->expire - Clock::now()).count() > 0) {
            break;
        }
        it->cb_func(it->user_data);
        timer_set.erase(it);
    }
}
