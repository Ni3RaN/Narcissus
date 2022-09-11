//
// Created by nie on 22-9-10.
//

#ifndef NEREIDS_SQL_CONNECTION_POOL_H
#define NEREIDS_SQL_CONNECTION_POOL_H

#include <mysql/mysql.h>
#include <error.h>
#include <string>
#include <list>
#include <memory>
#include "../lock/locker.h"

class sql_connection_pool {
public:
    std::shared_ptr<MYSQL> GetConnection();//获取数据库连接

    bool ReleaseConnection(std::shared_ptr<MYSQL> conn);//释放连接

    int GetFreeConn();//获取连接

    void DestroyPool();//销毁所有连接

    static std::shared_ptr<sql_connection_pool> GetInstance();//单例模式

    void init(std::string url, std::string User, std::string PassWord, std::string DataBaseName, int Port,
              int MaxConn, int close_log);//初始化数据库连接池

    ~sql_connection_pool();

private:
    sql_connection_pool();//构造函数
    //析构函数

    int m_MaxConn;//最大连接数
    int m_CurConn;//当前已使用的连接数
    int m_FreeConn;//当前空闲的连接数
    locker m_lock;//互斥锁
    std::list<std::shared_ptr<MYSQL> > connList;//连接池
    sem reserve;//信号量

public:
    std::string m_url;//主机地址
    int m_Port;//数据库端口号
    std::string m_User;//登陆数据库用户名
    std::string m_PassWord;//登陆数据库密码
    std::string m_DatabaseName;//使用数据库名
    int m_close_log;//日志开关
};

class connectionRAII {

public:
    connectionRAII(std::shared_ptr<std::shared_ptr<MYSQL> > SQL, std::shared_ptr<sql_connection_pool> connPool);

    ~connectionRAII();

private:
    std::shared_ptr<MYSQL> conRAII;
    std::shared_ptr<sql_connection_pool> poolRAII;
};

#endif //NEREIDS_SQL_CONNECTION_POOL_H
