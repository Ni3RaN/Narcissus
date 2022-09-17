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
    time_t expire{};

    //定时器回调函数
    void (*cb_func)(client_data *){};

    //回调函数处理的客户数据，由定时器的执行者传递给回调函数
    client_data *user_data{};

    //重载运算符，用于重置定时器
    bool operator<(const util_timer &rhs) const {
        return expire < rhs.expire;
    }
};

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

    bool shift_down_(size_t idx,size_t n);

    void SwapNode_(size_t i, size_t j);


};

class Utils {
public:
    Utils() {}

    ~Utils() {}

    //初始化
    void init(int timeslot);

    //将文件描述符设置为非阻塞
    int setnonblocking(int fd);

    //将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    //信号处理函数
    static void sig_handler(int sig);

    //设置信号函数
    void addsig(int sig, void(handler)(int), bool restart = true);

    //定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();

    //显示错误
    static void show_error(int connfd, const char *info);

public:
    static int *u_pipefd;//管道
    heap_timer timer_heap;//定时器小顶堆
    static int u_epollfd;//epoll fd
    int m_TIMESLOT;//定时时间


};

//定时器回调函数，它删除非活动连接socket上的注册事件，并关闭之
void cb_func(client_data *user_data);

#endif //NEREIDS_HEAP_TIMER_H
