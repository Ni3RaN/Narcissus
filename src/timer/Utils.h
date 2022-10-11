//
// Created by nie on 22-10-11.
//

#ifndef NEREIDS_UTILS_H
#define NEREIDS_UTILS_H

#include "set/set_timer.h"
#include "heap/heap_timer.h"

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
    set_timer timer_set;//set定时器
    heap_timer timer_heap;//heap定时器
    static int u_epollfd;//epoll fd
    int m_TIMESLOT;//定时时间
};

//定时器回调函数，它删除非活动连接socket上的注册事件，并关闭之
void cb_func(client_data *user_data);


#endif //NEREIDS_UTILS_H
