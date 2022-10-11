//
// Created by nie on 22-9-16.
//

#include "../../log/log.h"
#include "heap_timer.h"
#include "../../http/http_conn.h"


void heap_timer::shift_up_(size_t i) {
    assert(i >= 0 && i < heap_.size());
    size_t j = (i - 1) / 2;
    while (j >= 0) {
        if (heap_[j] < heap_[i]) {
            break;
        }
        SwapNode_(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}

void heap_timer::SwapNode_(size_t i, size_t j) {
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());
    std::swap(heap_[i], heap_[j]);
    ref_[heap_[i]->user_data] = i;
    ref_[heap_[j]->user_data] = j;
}

bool heap_timer::shift_down_(size_t idx, size_t n) {
    assert(idx >= 0 && idx < heap_.size());
    assert(n >= 0 && n <= heap_.size());
    size_t i = idx;
    size_t j = i * 2 + 1;
    while (j < n) {
        if (j + 1 < n && heap_[j + 1] < heap_[j]) {
            j = j + 1;
        }
        if (heap_[i] < heap_[j]) {
            break;
        }
        SwapNode_(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return i > idx;
}

void heap_timer::add_timer(util_timer *timer) {
    assert(timer != nullptr);
    size_t i;
    if (ref_.count(timer->user_data)) {
        i = ref_[timer->user_data];
        heap_[i]->expire = timer->expire;
        if (!shift_down_(i, heap_.size())) {
            shift_up_(i);
        }
    } else {
        i = heap_.size();
        heap_.push_back(timer);
        ref_[timer->user_data] = i;
        shift_up_(i);
    }
}

void heap_timer::doWork(util_timer *timer) {
    if (heap_.empty() || ref_.count(timer->user_data) == 0) {
        return;
    }
    timer->cb_func(timer->user_data);
    del_(ref_[timer->user_data]);
}

void heap_timer::del_timer(util_timer *timer) {
    del_(ref_[timer->user_data]);
}

void heap_timer::adjust_timer(util_timer *timer) {
    assert(!heap_.empty() && timer != nullptr);
    heap_[ref_[timer->user_data]]->expire = timer->expire;
    shift_down_(ref_[timer->user_data], heap_.size());
}

void heap_timer::tick() {
    if (heap_.empty()) {
        return;
    }
    while (!heap_.empty()) {
        util_timer *tmp = heap_.front();
        if (std::chrono::duration_cast<MS>(tmp->expire - Clock::now()).count() > 0) {
            break;
        }
        tmp->cb_func(tmp->user_data);
        pop_timer();
    }
}

void heap_timer::pop_timer() {
    assert(!heap_.empty());
    del_(0);
}

void heap_timer::init() {
    heap_.clear();
    ref_.clear();
}

int heap_timer::GetNextTick() {
    tick();
    size_t res = -1;
    if (!heap_.empty()) {
        res = std::chrono::duration_cast<MS>(heap_.front()->expire - Clock::now()).count();
        if (res < 0) {
            res = 0;
        }
    }
    return res;
}

void heap_timer::del_(size_t idx) {
    assert(!heap_.empty());
    assert(idx >= 0);
    assert(idx < heap_.size());
    size_t i = idx;
    size_t n = heap_.size() - 1;
    if (i < n) {
        SwapNode_(i, n);
        if (!shift_down_(i, n)) {
            shift_up_(i);
        }
    }
    ref_.erase(heap_.back()->user_data);
    heap_.pop_back();
}
