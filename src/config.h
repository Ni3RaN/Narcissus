#ifndef CONFIG_H
#define CONFIG_H

#include "webserver.h"

using namespace std;

class Config
{
public:
    Config();
    ~Config() = default;

    //端口号
    int port;

    //日志写入方式
    int log_write;

    //触发组合模式
    int trig_mode;

    //listenfd触发模式
    int listen_trig_mode;

    //connfd触发模式
    int conn_trig_mode;

    //优雅关闭链接
    int opt_linger;

    //数据库连接池数量
    int sql_num;

    //线程池内的线程数量
    int thread_num;

    //是否关闭日志
    int close_log;

    //数据库端口
    int mysql_port;

    int timer_mode;

    //数据库url
    std::string mysql_url;

    //数据库用户名
    std::string mysql_username;

    //数据库密码
    std::string mysql_password;

    //选择的数据库
    std::string mysql_database;

};

#endif