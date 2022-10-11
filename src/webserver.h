//
// Created by nie on 22-9-7.
//

#ifndef NEREIDS_WEBSERVER_H
#define NEREIDS_WEBSERVER_H


#include <sys/socket.h>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <cstring>
#include <netinet/in.h>

#include "threadpool/threadpool.h"
#include "http/http_conn.h"
#include "timer/Utils.h"

const int MAX_FD = 65536; //最大文件描述符
const int TIMESLOT = 5000; //最小超时单位,单位ms
const int MAX_EVENT_NUMBER = 10000; //最大事件数

class WebServer {

public:
    WebServer();

    ~WebServer();

    void init(int port, int sql_port, std::string sql_url, std::string sql_username, std::string sql_password,
              std::string sql_database, int log_write,
              int opt_linger, int trig_mode, int sql_num, int thread_num, int close_log, int timer_mode);

    void thread_pool();

    void sql_pool();

    void log_write();

    void trig_mode();

    void eventListen();

    void eventLoop();

    void timer(int connfd, struct sockaddr_in client_address);

    void adjust_timer(util_timer *timer);

    void deal_timer(util_timer *timer, int sockfd);

    bool dealclinetdata();

    bool dealwithsignal(bool &timeout, bool &stop_server);

    void dealwithread(int sockfd);

    void dealwithwrite(int sockfd);


public:
    int m_port{};//端口号
    char *m_root{};//根目录

    int m_log_write{};//日志写入方式
    int m_close_log{};//是否关闭日志
    int m_pipefd[2]{};//管道
    int m_epollfd{};//epoll句柄
    http_conn *users{};//用户数组

    //数据库
    int m_sql_num{}; //数据库连接池数量
    sql_connection_pool *m_connPool{}; //数据库连接池
    std::string mysql_url; //数据库url
    int mysql_port; //数据库端口
    std::string mysql_username; //数据库用户名
    std::string mysql_password; //数据库密码
    std::string mysql_database; //数据库名

    threadpool<http_conn> *m_pool{};//线程池
    int m_thread_num{};//线程池数量

    epoll_event events[MAX_EVENT_NUMBER]{}; //epoll事件数组

    int m_opt_linger{};//是否优雅关闭连接
    int m_listenfd{};//监听套接字
    int m_triggermode{};//触发模式
    int m_listenTriggermode{};//监听触发模式
    int m_connTriggermode{};//连接触发模式
    int m_timer_mode{};//定时器模式

    client_data *users_timer{};//用户数组

    Utils utils; //工具类

};


#endif //NEREIDS_WEBSERVER_H
