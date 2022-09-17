//
// Created by nie on 22-9-16.
//

#include "../log/log.h"
#include "heap_timer.h"


void heap_timer::shift_up_(size_t i) {
    assert(i >= 0 && i < heap_.size());
    size_t j = (i - 1) / 2;
    while (j >= 0) {
        if (heap_[j] < heap_[i]) {
            break;
        }
        std::swap(heap_[j], heap_[i]);
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
        shift_down_(i, heap_.size());
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
    del_timer(timer);
}

void heap_timer::del_timer(util_timer *timer) {
    assert(!heap_.empty() && timer != nullptr);
    if (ref_.count(timer->user_data) == 0) {
        return;
    }
    size_t i = ref_[timer->user_data];
    size_t n = heap_.size() - 1;
    assert(i <= n);
    if (i < n) {
        SwapNode_(i, n);
        if (!shift_down_(i, n)) {
            shift_up_(i);
        }
    }
    heap_.pop_back();
    ref_.erase(timer->user_data);
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
        if (tmp->expire > time(nullptr)) {
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
        res = heap_.front()->expire - time(nullptr);
        if (res < 0) {
            res = 0;
        }
    }
    return res;
}

/**
 * 初始化
 * @param timeslot 时间间隔
 */
void Utils::init(int timeslot) {
    m_TIMESLOT = timeslot;
}

/**
 * 对文件描述符设置非阻塞
 * @param fd 文件描述符
 * @return 文件读写状态
 */
int Utils::setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

/**
 * 将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
 * @param epollfd epoll文件描述符
 * @param fd 文件描述符
 * @param one_shot 是否开启EPOLLONESHOT
 * @param TRIGMode 触发模式
 */
void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode) {
    epoll_event event{};
    event.data.fd = fd;
    if (1 == TRIGMode) {
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    } else {
        event.events = EPOLLIN | EPOLLRDHUP;
    }
    if (one_shot) {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

/**
 * 信号处理
 * @param sig 信号
 */
void Utils::sig_handler(int sig) {
    //为保证函数的可重入性，保留原来的errno
    int save_errno = errno;
    int msg = sig;
    send(u_pipefd[1], (char *) &msg, 1, 0);
    errno = save_errno;
}

/**
 * 设置信号函数
 * @param sig 信号
 * @param handler 信号处理函数
 * @param restart 是否重新启动系统调用
 */
void Utils::addsig(int sig, void(handler)(int), bool restart) {
    struct sigaction sa{};
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart) {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, nullptr) != -1);
}

/**
 * 定时器回调函数，它删除非活动连接socket上的注册事件，并关闭之
 */
void Utils::timer_handler() {
    timer_heap.tick();
    //最小的时间单位为5s
    alarm(m_TIMESLOT);
}

/**
 * 定时器回调函数，它关闭连接socket上的注册事件，并关闭之
 * @param connfd 连接socket
 * @param info 客户端信息
 */
void Utils::show_error(int connfd, const char *info) {
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int *Utils::u_pipefd = nullptr;
int Utils::u_epollfd = 0;

class Utils;

void cb_func(client_data *user_data) {
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    int m_close_log = 1;
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
