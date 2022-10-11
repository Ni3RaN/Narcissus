#include <fstream>
#include <iostream>
#include "config.h"

Config::Config(){

    std::ifstream ifile("config.json");
    if(!ifile.is_open()){
        std::cout<<"config file open error"<<std::endl;
        exit(1);
    }
    Json::CharReaderBuilder ReaderBuilder;
    ReaderBuilder["emitUTF8"] = true;
    Json::Value json;
    std::string error;

    bool flag = Json::parseFromStream(ReaderBuilder,ifile,&json, &error);
    if(!flag) {
        std::cout<<error<<"\n";
        exit(0);
    }

    //端口号
    port = json["config"]["port"].asInt();

    //日志写入方式，true 同步,false 异步
    log_write = json["config"]["log_write"].asBool();

    //触发组合模式，true LT + LT, false LT + ET
    trig_mode = json["config"]["trig_mode"].asBool();

    //listenfd触发模式，true ET, false LT
    listen_trig_mode = json["config"]["listen_trig_mode"].asBool();

    //connfd触发模式，true ET, false LT
    conn_trig_mode = json["config"]["conn_trig_mode"].asBool();

    //优雅关闭链接，true 优雅关闭, false 正常关闭
    opt_linger = json["config"]["opt_linger"].asBool();

    //数据库连接池数量，默认为8
    sql_num = json["config"]["sql_num"].asInt();

    //线程池内的线程数量，默认为8
    thread_num = json["config"]["thread_num"].asInt();

    //true 关闭日志,false 不关闭日志
    close_log = json["config"]["close_log"].asBool();

    //数据库端口，默认3306
    mysql_port = json["config"]["mysql"]["port"].asInt();

    //数据库url，默认localhost
    mysql_url = json["config"]["mysql"]["url"].asString();

    //数据库用户名
    mysql_username = json["config"]["mysql"]["username"].asString();

    //数据库密码
    mysql_password = json["config"]["mysql"]["password"].asString();

    //选择的数据库
    mysql_database = json["config"]["mysql"]["database"].asString();

    //定时器模式,0 heap, 1 set
    timer_mode = json["config"]["heap_mode"].asInt();

}
