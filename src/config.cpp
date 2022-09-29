#include <fstream>
#include "config.h"

Config::Config(){

    std::ifstream ifile("config.json");
    Json::CharReaderBuilder ReaderBuilder;
    ReaderBuilder["emitUTF8"] = true;
    Json::Value json;
    std::string error;

    bool flag = Json::parseFromStream(ReaderBuilder,ifile,&json, &error);
    if(!flag) {
        std::cout<<error<<"\n";
        exit(0);
    }

    port = json["config"]["port"].asInt();

    //日志写入方式，true 同步,false 异步
    log_write = json["config"]["log_write"].asBool();


    trig_mode = json["config"]["trig_mode"].asBool();

    listen_trig_mode = json["config"]["listen_trig_mode"].asBool();

    conn_trig_mode = json["config"]["conn_trig_mode"].asBool();

    opt_linger = json["config"]["opt_linger"].asBool();

    sql_num = json["config"]["sql_num"].asBool();

    thread_num = json["config"]["thread_num"].asBool();

    //true 关闭日志,false 不关闭日志
    close_log = json["config"]["close_log"].asBool();

    mysql_port = json["config"]["mysql"]["port"].asInt();

    mysql_url = json["config"]["mysql"]["url"].asString();

    mysql_username = json["config"]["mysql"]["username"].asString();

    mysql_password = json["config"]["mysql"]["password"].asString();


    mysql_database = json["config"]["mysql"]["database"].asString();

//    //端口号,默认9000
//    PORT = 9000;
//
//    //日志写入方式，默认同步
//    LOGWrite = 0;
//
//    //触发组合模式,默认listenfd LT + connfd LT
//    TRIGMode = 0;
//
//    //listenfd触发模式，默认LT
//    LISTENTrigmode = 0;
//
//    //connfd触发模式，默认LT
//    CONNTrigmode = 0;
//
//    //优雅关闭链接，默认不使用
//    OPT_LINGER = 0;
//
//    //数据库连接池数量,默认8
//    sql_num = 8;
//
//    //线程池内的线程数量,默认8
//    thread_num = 8;
//

}
