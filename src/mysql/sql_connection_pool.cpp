//
// Created by nie on 22-9-10.
//

#include "sql_connection_pool.h"
#include "../log/log.h"


sql_connection_pool::sql_connection_pool() {
    m_CurConn = 0;
    m_FreeConn = 0;
}

std::shared_ptr<sql_connection_pool> sql_connection_pool::GetInstance() {
    static std::shared_ptr<sql_connection_pool> connPool(new sql_connection_pool);
    return connPool;
}

/**
 * 初始化数据库连接池
 * @param url 主机地址
 * @param User 登陆数据库用户名
 * @param PassWord 登陆数据库密码
 * @param DataBaseName 使用数据库名
 * @param Port 数据库端口号
 * @param MaxConn 最大连接数
 */
void sql_connection_pool::init(std::string url, std::string User, std::string PassWord, std::string DataBaseName,
                               int Port, int MaxConn, int close_log) {
    m_url = std::move(url);
    m_Port = Port;
    m_User = std::move(User);
    m_PassWord = std::move(PassWord);
    m_DatabaseName = std::move(DataBaseName);
    m_MaxConn = MaxConn;
    m_close_log = close_log;

    for (int i = 0; i < MaxConn; i++) {
        std::shared_ptr<MYSQL> conn(mysql_init(nullptr), mysql_close);
        if (conn == nullptr) {
            LOG_ERROR("MySQL Error");
            exit(1);
        }
        conn = static_cast<const std::shared_ptr<MYSQL>>(mysql_real_connect(conn.get(), m_url.c_str(), m_User.c_str(),
                                                                            m_PassWord.c_str(), m_DatabaseName.c_str(),
                                                                            m_Port, nullptr, 0));

        if (conn == nullptr) {
            LOG_ERROR("MySQL Error");
            exit(1);
        }
        connList.push_back(conn);
        m_FreeConn++;
    }
    reserve = sem(m_FreeConn);
    m_MaxConn = m_FreeConn;
}

/**
 * 当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
 * @return 可用连接
 */
std::shared_ptr<MYSQL> sql_connection_pool::GetConnection() {
    std::shared_ptr<MYSQL> conn = nullptr;
    if (connList.empty()) {
        return conn;
    }
    reserve.wait();
    m_lock.lock();
    conn = connList.front();
    connList.pop_front();
    m_FreeConn--;
    m_CurConn++;
    m_lock.unlock();
    return conn;
}

/**
 * 释放当前使用的连接
 * @param conn 当前使用的连接
 * @return 释放成功返回true
 */
bool sql_connection_pool::ReleaseConnection(std::shared_ptr<MYSQL> conn) {
    if (nullptr == conn) {
        return false;
    }
    m_lock.lock();
    connList.push_back(conn);
    m_FreeConn++;
    m_CurConn--;
    m_lock.unlock();
    reserve.post();
    return true;
}

/**
 * 销毁数据库连接池
 */
void sql_connection_pool::DestroyPool() {
    m_lock.lock();
    if (!connList.empty()) {
        for (auto &conn: connList) {
            mysql_close(conn.get());
        }
        m_CurConn = 0;
        m_FreeConn = 0;
        connList.clear();
    }
    m_lock.unlock();
}

/**
 * 当前空闲的连接数
 * @return 空闲的连接数
 */
int sql_connection_pool::GetFreeConn() {
    return this->m_FreeConn;
}

/**
 * 销毁数据库连接池
 */
sql_connection_pool::~sql_connection_pool() {
    DestroyPool();
}

connectionRAII::connectionRAII(std::shared_ptr<std::shared_ptr<MYSQL>> SQL,
                               std::shared_ptr<sql_connection_pool> connPool) {
    *SQL = connPool->GetConnection();
    conRAII = *SQL;
    poolRAII = connPool;
}

connectionRAII::~connectionRAII() {
    poolRAII->ReleaseConnection(conRAII);
}
