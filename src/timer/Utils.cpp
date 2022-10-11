//
// Created by nie on 22-10-11.
//

#include "Utils.h"
#include "../http/http_conn.h"

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
    timer_set.tick();
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
    http_conn::m_user_count--;
}