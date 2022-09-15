//
// Created by nie on 22-9-7.
//

#include "../log/log.h"
#include "lst_timer.h"

/**
 * 构造函数
 */
sort_timer_lst::sort_timer_lst() {
    set.clear();
}

/**
 * 析构函数
 */
sort_timer_lst::~sort_timer_lst() {
    set.clear();
}

/**
 * 将目标定时器timer添加到链表中
 * @param timer 目标定时器
 */
void sort_timer_lst::add_timer(util_timer *timer) {
    if (timer == nullptr) {
        return;
    }
    set.insert(timer);
}

/**
 * 从set中删除目标定时器timer
 * @param timer 目标定时器
 */
void sort_timer_lst::del_timer(util_timer *timer) {
    if (timer == nullptr) {
        return;
    }
    auto it = set.find(timer);
    if (it != set.end()) {
        set.erase(*it);
    }
}

/**
 * SIGALRM信号每次被触发就在其信号处理函数中执行一次tick函数，以处理set上到期的任务
 */
void sort_timer_lst::tick() {
    //如果set为空，则直接返回
    if (set.empty()) {
        return;
    }
    //获取当前系统时间
    auto cur = time(nullptr);
    //获取set中第一个定时器
    auto it = set.begin();
    //遍历set中的定时器
    while (it != set.end()) {
        //如果当前定时器的到期时间大于当前系统时间，则说明后面的定时器也都还未到期，直接返回
        if ((*it)->expire > cur) {
            break;
        }
        //否则，说明当前定时器已经到期，调用其回调函数，执行定时任务
        (*it)->cb_func((*it)->user_data);
        //删除当前定时器
        it = set.erase(it);
        it++;
    }
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
    m_timer_lst.tick();
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
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockdfd, 0);
    assert(user_data);
    close(user_data->sockdfd);
    std::cout << "close fd " << user_data->sockdfd << std::endl;
//    LOG_INFO("close fd %d", user_data->sockdfd);
}