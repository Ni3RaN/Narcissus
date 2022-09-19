//
// Created by nie on 22-9-8.
//

#ifndef NEREIDS_HTTP_CONN_H
#define NEREIDS_HTTP_CONN_H


#include <unistd.h>
#include <csignal>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cassert>
#include <sys/stat.h>
#include <cstring>
#include <pthread.h>
#include <cstdio>
#include <cstdlib>
#include <sys/mman.h>
#include <cstdarg>
#include <cerrno>
#include <sys/wait.h>
#include <sys/uio.h>
#include <map>
#include <jsoncpp/json/json.h>

#include "../lock/locker.h"
#include "../mysql/sql_connection_pool.h"
#include "../timer/heap_timer.h"
#include "../log/log.h"


class http_conn {
public:
    static const int FILENAME_LEN = 200; //文件名的最大长度
    static const int READ_BUFFER_SIZE = 2048; //读缓冲区的大小
    static const int WRITE_BUFFER_SIZE = 1024; //写缓冲区的大小

    //HTTP请求方法，但我们仅支持GET
    enum METHOD {
        GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATCH
    };

    //解析客户请求时，主状态机所处的状态
    enum CHECK_STATE {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };

    //服务器处理HTTP请求的可能结果
    enum HTTP_CODE {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        JSON_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };

    //行的读取状态
    enum LINE_STATUS {
        LINE_OK = 0, LINE_BAD, LINE_OPEN
    };

public:
    http_conn() {}

    ~http_conn() {}

public:
    //初始化新接受的连接
    void init(int sockfd, const sockaddr_in &addr, char *root, int trigmode, int close_log, std::string user,
              std::string passWord, std::string sqlname);

    //关闭连接
    void close_conn(bool real_close = true);

    //处理客户请求
    void process();

    //非阻塞读操作
    bool read_once();

    //非阻塞写操作
    bool write();

    //获取客户端的socket地址
    sockaddr_in *get_address() {
        return &m_address;
    }

    //初始化数据库读取表
    void initmysql_result(sql_connection_pool *connectionPool);

    int timer_flag;//是否关闭连接
    int improv;//是否正在处理数据中
private:
    //初始化连接
    void init();

    //解析HTTP请求
    HTTP_CODE process_read();

    //填充HTTP应答
    bool process_write(HTTP_CODE ret);

    //下面这组函数被process_read调用以分析HTTP请求
    HTTP_CODE parse_request_line(char *text);

    //解析请求头
    HTTP_CODE parse_headers(char *text);

    //解析请求体
    HTTP_CODE parse_content(char *text);

    //生成响应
    HTTP_CODE do_request();

    //获取文件名
    char *get_line() { return m_read_buf + m_start_line; }

    //获取行状态
    LINE_STATUS parse_line();

    //unmap
    void unmap();

    //request
    bool add_response(char *format, ...);

    bool add_content(char *content);

    bool add_status_line(int status, char *title);

    bool add_headers(int content_length);

    bool add_content_type();

    bool add_content_length(int content_length);

    bool add_linger();

    bool add_blank_line();

public:
    static int m_epollfd;
    static int m_user_count;
    MYSQL *mysql;
    int m_state;//读为0，写为1

private:
    //该HTTP连接的socket和对方的socket地址
    int m_sockfd;
    //socket地址
    sockaddr_in m_address;
    //读缓冲区
    char m_read_buf[READ_BUFFER_SIZE];
    //标识读缓冲中已经读入的客户数据的最后一个字节的下一个位置
    int m_read_idx;
    //当前正在分析的字符在读缓冲区中的位置
    int m_checked_idx;
    //当前正在解析的行的起始位置
    int m_start_line;
    //写缓冲区
    char m_write_buf[WRITE_BUFFER_SIZE];
    //写缓冲区中待发送的字节数
    int m_write_idx;
    //主状态机当前所处的状态
    CHECK_STATE m_check_state;
    //请求方法
    METHOD m_method;
    //客户请求的目标文件的完整路径，其内容等于doc_root + m_url, doc_root是网站根目录
    char m_real_file[FILENAME_LEN];
    //客户请求的目标文件的url
    char *m_url;
    //HTTP协议版本号
    char *m_version;
    char *m_host;
    int m_content_length;
    bool m_linger;
    char *m_file_address;
    struct stat m_file_stat;
    struct iovec m_iv[2];
    int m_iv_count;
    int cgi;        //是否启用的POST
    char *m_string; //存储请求头数据
    char m_json[1024];   //存储json数据
    //剩余发送字节数
    int bytes_to_send;
    //已发送字节数
    int bytes_have_send;
    char *doc_root;

    std::map<std::string, std::string> m_users;//用户名密码匹配表
    int m_TRIGMode;//触发模式
    int m_close_log;//是否开启日志

    char sql_user[100];
    char sql_passwd[100];
    char sql_name[100];

};


#endif //NEREIDS_HTTP_CONN_H
