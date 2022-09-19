//
// Created by nie on 22-9-8.
//

#include "http_conn.h"

#include <mysql/mysql.h>


//定义http响应的一些状态信息
char *ok_200_title = "OK";
char *error_400_title = "Bad Request";
char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
char *error_403_title = "Forbidden";
char *error_403_form = "You do not have permission to get file form this server.\n";
char *error_404_title = "Not Found";
char *error_404_form = "The requested file was not found on this server.\n";
char *error_500_title = "Internal Error";
char *error_500_form = "There was an unusual problem serving the request file.\n";


locker m_lock;
std::map<std::string, std::string> users;
int http_conn::m_user_count = 0;
int http_conn::m_epollfd = -1;


void http_conn::initmysql_result(sql_connection_pool *connectionPool) {
    MYSQL *mysql = nullptr;
    connectionRAII mysqlcon(&mysql, connectionPool);

    //在user表中检索username，passwd数据，浏览器端输入的信息将与之进行匹配
    if (mysql_query(mysql, "SELECT username,passwd FROM user")) {
        LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
    }

    //从表中检索完整的结果集
    MYSQL_RES *result = mysql_store_result(mysql);
    //返回结果集中的列数
    int num_fields = mysql_num_fields(result);

    MYSQL_FIELD *fields = mysql_fetch_fields(result);

    //从结果集中获取下一行，将对应的用户名和密码，存入map中
    while (MYSQL_ROW row = mysql_fetch_row(result)) {
        std::string temp1(row[0]);
        std::string temp2(row[1]);
        users[temp1] = temp2;
    }
}

//对文件描述符设置非阻塞
int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void addfd(int epollfd, int fd, bool one_shot, int TRIGMode) {
    epoll_event event{};
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

//从内核时间表删除描述符
void removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
}


//将事件重置为EPOLLONESHOT
void modfd(int epollfd, int fd, int ev, int TRIGMode) {
    epoll_event event{};
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    else
        event.events = ev | EPOLLONESHOT | EPOLLRDHUP;

    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

/**
 * 关闭连接，关闭一个连接，客户总量减1
 * @param real_close 是否真正关闭连接
 */
void http_conn::close_conn(bool real_close) {
    if (real_close && (m_sockfd != -1)) {
        LOG_INFO("close %d", m_sockfd);
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        m_user_count--;
    }
}

/**
 * 初始化新接受的连接
 * @param sockfd
 * @param addr
 * @param root
 * @param trigmode
 * @param close_log
 * @param user
 * @param passWord
 * @param sqlname
 */
void http_conn::init(int sockfd, const sockaddr_in &addr, char *root, int trigmode, int close_log, std::string user,
                     std::string passWord, std::string sqlname) {
    m_sockfd = sockfd;
    m_address = addr;

    addfd(m_epollfd, sockfd, true, m_TRIGMode);
    m_user_count++;

    doc_root = root;
    m_TRIGMode = trigmode;
    m_close_log = close_log;

    strcpy(sql_user, user.c_str());
    strcpy(sql_passwd, passWord.c_str());
    strcpy(sql_name, sqlname.c_str());

    init();
}

/**
 * 初始化新接受的连接
 */
void http_conn::init() {
    mysql = nullptr;
    bytes_to_send = 0;
    bytes_have_send = 0;
    m_check_state = CHECK_STATE_REQUESTLINE;
    m_linger = false;
    m_method = GET;
    m_url = nullptr;
    m_version = nullptr;
    m_content_length = 0;
    m_host = nullptr;
    m_start_line = 0;
    m_checked_idx = 0;
    m_read_idx = 0;
    m_write_idx = 0;
    cgi = 0;
    m_state = 0;
    timer_flag = 0;
    improv = 0;

    memset(m_read_buf, '\0', READ_BUFFER_SIZE);
    memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
    memset(m_real_file, '\0', FILENAME_LEN);
}

/**
 * 从状态机，用于分析出一行内容
 * @return 返回值为行的读取状态，有LINE_OK,LINE_BAD,LINE_OPEN
 */
http_conn::LINE_STATUS http_conn::parse_line() {
    for (; m_checked_idx < m_read_idx; m_checked_idx++) {
        char tmp = m_read_buf[m_checked_idx];
        if (tmp == '\r') {
            if ((m_checked_idx + 1) == m_read_idx) {
                return LINE_OPEN;
            } else if (m_read_buf[m_checked_idx + 1] == '\n') {
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        } else if (tmp == '\n') {
            if ((m_checked_idx > 1) && (m_read_buf[m_checked_idx - 1] == '\r')) {
                m_read_buf[m_checked_idx - 1] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

/**
 * 循环读取客户数据，直到无数据可读或对方关闭连接
 * 非阻塞ET工作模式下，需要一次性将数据读完
 * @return 是否读取成功
 */
bool http_conn::read_once() {
    if (m_read_idx >= READ_BUFFER_SIZE) {
        return false;
    }
    int bytes_read = 0;
    //LT读取数据
    if (0 == m_TRIGMode) {
        bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
        m_read_idx += bytes_read;
        if (bytes_read <= 0) {
            return false;
        }
        return true;
    }
        //ET读数据
    else {
        while (true) {
            bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
            if (bytes_read == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                return false;
            } else if (bytes_read == 0) {
                return false;
            }
            m_read_idx += bytes_read;
        }
        return true;
    }
}

/**
 * 解析HTTP请求行，获得请求方法，目标URL，以及HTTP版本号
 * @param text 请求行
 * @return 解析结果
 */
http_conn::HTTP_CODE http_conn::parse_request_line(char *text) {
    //在HTTP报文中，请求行用来说明请求类型
    //要访问的资源以及所使用的HTTP版本，其中各个部分之间通过\t或空格分隔。
    //请求行中最先含有空格和\t任一个字符的位置并返回

    //strpbrk在源字符串（s1）中找出最先含有搜索字符串（s2）
    //中任一个字符的位置并返回，若找不到则返回空指针
    m_url = strpbrk(text, " \t");
    //如果请求行中没有空格或\t字符，则HTTP请求必有问题
    if (!m_url) {
        return BAD_REQUEST;
    }
    //将空格或\t字符改为\0，用于后面的分割字符串
    *m_url++ = '\0';

    char *method = text;
    if (strcasecmp(method, "GET") == 0) {
        m_method = GET;
    } else if (strcasecmp(method, "POST") == 0) {
        m_method = POST;
        cgi = 1;
    } else {
        return BAD_REQUEST;
    }

    m_url += strspn(m_url, " \t");
    m_version = strpbrk(m_url, " \t");
    if (!m_version) {
        return BAD_REQUEST;
    }
    *m_version++ = '\0';
    m_version += strspn(m_version, " \t");
    if (strcasecmp(m_version, "HTTP/1.1") != 0) {
        return BAD_REQUEST;
    }
    if (strncasecmp(m_url, "http://", 7) == 0) {
        m_url += 7;
        m_url = strchr(m_url, '/');
    }
    if (strncasecmp(m_url, "https://", 8) == 0) {
        m_url += 8;
        m_url = strchr(m_url, '/');
    }
    if (!m_url || m_url[0] != '/') {
        return BAD_REQUEST;
    }
    if (strlen(m_url) == 1) {
        return BAD_REQUEST;
    }
    m_check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

/**
 * 解析HTTP请求的一个头部信息
 * @param text 头部信息
 * @return 解析结果
 */
http_conn::HTTP_CODE http_conn::parse_headers(char *text) {
    //遇到一个空行，表示头部字段解析完毕
    if (text[0] == '\0') {
        //如果HTTP请求有消息体，则还需要读取m_content_length字节的消息体，状态机转移到CHECK_STATE_CONTENT状态
        if (m_content_length != 0) {
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        //否则说明我们已经得到了一个完整的HTTP请求
        return GET_REQUEST;
    } else if (strncasecmp(text, "Connection:", 11) == 0) {
        text += 11;
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0) {
            m_linger = true;
        }
    } else if (strncasecmp(text, "Content-Length:", 15) == 0) {
        text += 15;
        text += strspn(text, " \t");
        m_content_length = atol(text);
    } else if (strncasecmp(text, "Host:", 5) == 0) {
        text += 5;
        text += strspn(text, " \t");
        m_host = text;
    } else {
        LOG_INFO("oop! unknow header %s", text);
    }
    return NO_REQUEST;
}

/**
 * 我们没有真正解析HTTP请求的消息体，只是判断它是否被完整地读入了
 * @return 解析结果
 */
http_conn::HTTP_CODE http_conn::parse_content(char *text) {
    if (m_read_idx >= (m_content_length + m_checked_idx)) {
        text[m_content_length] = '\0';
        m_string = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

/**
 * 主状态机
 * @return 解析结果
 */
http_conn::HTTP_CODE http_conn::process_read() {
    LINE_STATUS lineStatus = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char *text = nullptr;
    while (((m_check_state == CHECK_STATE_CONTENT) && (lineStatus == LINE_OK)) ||
           ((lineStatus = parse_line()) == LINE_OK)) {
        text = get_line();
        m_start_line = m_checked_idx;
        LOG_INFO("%s", text);
        switch (m_check_state) {
            case CHECK_STATE_REQUESTLINE: {
                ret = parse_request_line(text);
                if (ret == BAD_REQUEST) {
                    return BAD_REQUEST;
                }
                break;
            }
            case CHECK_STATE_HEADER: {
                ret = parse_headers(text);
                if (ret == BAD_REQUEST) {
                    return BAD_REQUEST;
                } else if (ret == GET_REQUEST) {
                    return do_request();
                }
                break;
            }
            case CHECK_STATE_CONTENT: {
                ret = parse_content(text);
                if (ret == GET_REQUEST) {
                    return do_request();
                }
                lineStatus = LINE_OPEN;
                break;
            }
            default: {
                return INTERNAL_ERROR;
            }
        }
    }
    return NO_REQUEST;
}

static unsigned char dec_tab[256] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,
        0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

/**
 * URL 解码函数
 * @param str {const char*} 经URL编码后的字符串
 * @return {char*} 解码后的字符串，返回值不可能为空，需要用 free 释放
 */
char *acl_url_decode(char *str) {
    int len = (int) strlen(str);
    char *tmp = (char *) malloc(len + 1);

    int i = 0, pos = 0;
    for (i = 0; i < len; i++) {
        if (str[i] != '%')
            tmp[pos] = str[i];
        else if (i + 2 >= len) {  /* check boundary */
            tmp[pos++] = '%';  /* keep it */
            if (++i >= len)
                break;
            tmp[pos] = str[i];
            break;
        } else if (isalnum(str[i + 1]) && isalnum(str[i + 2])) {
            tmp[pos] = (dec_tab[(unsigned char) str[i + 1]] << 4)
                       + dec_tab[(unsigned char) str[i + 2]];
            i += 2;
        } else
            tmp[pos] = str[i];

        pos++;
    }

    tmp[pos] = '\0';
    return tmp;
}


/**
 * 当得到一个完整、正确的HTTP请求时，我们就分析目标文件的属性。如果目标文件存在、对所有用户可读，且不是目录，则使用mmap将其映射到内存地址m_file_address处，并告诉调用者获取文件成功
 * @return 文件是否存在
 */
http_conn::HTTP_CODE http_conn::do_request() {
    strcpy(m_real_file, doc_root);
    int len = strlen(doc_root);
    //post请求
    if (m_method == POST) {
        const char *p = strchr(m_url, '/');
        if (p == nullptr) {
            return BAD_REQUEST;
        }
        if (strcmp(p, "/nereids/sign_in") == 0) {
            //将用户名和密码提取出来
            //user=123&passwd=123
            char name[100], password[100];
            int i;
            for (i = 5; m_string[i] != '&'; i++) {
                name[i - 5] = m_string[i];
            }
            name[i - 5] = '\0';

            int j = 0;
            for (i = i + 10; m_string[i] != '\0'; i++, j++) {
                password[j] = m_string[i];
            }
            password[j] = '\0';
            char *n_name = acl_url_decode(name);
            char *n_password = acl_url_decode(password);
            Json::Value json_ret;
            if (users.find(n_name) != users.end() && users[n_name] == n_password) {
                json_ret["status"] = 200;
                json_ret["sign_in"] = Json::Value("true");
                json_ret["msg"] = "login success";
            } else {
                json_ret["status"] = 200;
                json_ret["sign_in"] = Json::Value("false");
                json_ret["msg"] = "login failed";
            }
            Json::StyledWriter writer;
            std::string json_file = writer.write(json_ret);
            strcpy(m_json, json_file.c_str());
            free(n_name);
            free(n_password);
            return JSON_REQUEST;
        } else {
            return BAD_REQUEST;
        }
    }
        //get请求
    else {
        return BAD_REQUEST;
    }
//    if (stat(m_real_file, &m_file_stat) < 0)
//        return NO_RESOURCE;
//
//    if (!(m_file_stat.st_mode & S_IROTH))
//        return FORBIDDEN_REQUEST;
//
//    if (S_ISDIR(m_file_stat.st_mode))
//        return BAD_REQUEST;
//
//    int fd = open(m_real_file, O_RDONLY);
//    m_file_address = (char *) mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
//    close(fd);
//    return FILE_REQUEST;
}

/**
 * 取消内存映射
 */
void http_conn::unmap() {
    if (m_file_address) {
        munmap(m_file_address, m_file_stat.st_size);
        m_file_address = 0;
    }
}

/**
 * 写HTTP响应
 */
bool http_conn::write() {
    int temp = 0;
    if (bytes_to_send == 0) {
        modfd(m_epollfd, m_sockfd, EPOLLIN, m_TRIGMode);
        init();
        return true;
    }
    while (true) {
        temp = writev(m_sockfd, m_iv, m_iv_count);
        if (temp < 0) {
            if (errno == EAGAIN) {
                modfd(m_epollfd, m_sockfd, EPOLLOUT, m_TRIGMode);
                return true;
            }
            unmap();
            return false;
        }
        bytes_have_send += temp;
        bytes_to_send -= temp;
        if (bytes_have_send >= m_iv[0].iov_len) {
            m_iv[0].iov_len = 0;
            m_iv[1].iov_base = m_file_address + (bytes_have_send - m_write_idx);
            m_iv[1].iov_len = bytes_to_send;
        } else {
            m_iv[0].iov_base = m_write_buf + bytes_have_send;
            m_iv[0].iov_len = m_iv[0].iov_len - bytes_have_send;
        }
        if (bytes_to_send <= 0) {
            unmap();
            modfd(m_epollfd, m_sockfd, EPOLLIN, m_TRIGMode);
            if (m_linger) {
                init();
                return true;
            } else {
                return false;
            }
        }
    }
}

/**
 * 添加响应报文的公共函数
 */
bool http_conn::add_response(char *format, ...) {
    if (m_write_idx >= WRITE_BUFFER_SIZE)
        return false;
    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - 1 - m_write_idx, format, arg_list);
    if (len >= (WRITE_BUFFER_SIZE - 1 - m_write_idx))
        return false;
    m_write_idx += len;
    va_end(arg_list);
    LOG_INFO("request:%s", m_write_buf);
    return true;
}

/**
 * 添加状态行
 * @param status HTTP响应状态码
 * @param title 状态码对应的短描述
 * @return 是否添加成功
 */
bool http_conn::add_status_line(int status, char *title) {
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

/**
 * 添加消息报头，具体的添加文本长度、连接状态和空行
 * @return 是否添加成功
 */
bool http_conn::add_headers(int content_len) {
    return add_content_length(content_len) && add_linger() && add_blank_line();
}

/**
 * 添加文本长度
 * @param content_len 文本长度
 * @return 是否添加成功
 */
bool http_conn::add_content_length(int content_len) {
    return add_response("Content-Length: %d\r\n", content_len);
}

/**
 * 添加连接状态，通知浏览器端是保持连接还是关闭连接
 * @return 是否添加成功
 */
bool http_conn::add_content_type() {
    return add_response("Content-Type:%s\r\n", "text/html");
}

/**
 * 添加连接状态，通知浏览器端是保持连接还是关闭连接
 * @return 是否添加成功
 */
bool http_conn::add_linger() {
    return add_response("Connection: %s\r\n", m_linger ? "keep-alive" : "close");
}

/**
 * 添加空行
 * @return 是否添加成功
 */
bool http_conn::add_blank_line() {
    return add_response("%s", "\r\n");
}

/**
 * 添加文本内容
 * @param content 文本内容
 * @return 是否添加成功
 */
bool http_conn::add_content(char *content) {
    return add_response("%s", content);
}

/**
 * 根据服务器处理HTTP请求的结果，决定返回给客户端的内容
 * @param status HTTP响应状态码
 * @param title 状态码对应的短描述
 * @param content 需要返回的内容
 * @return 是否添加成功
 */
bool http_conn::process_write(HTTP_CODE ret) {
    switch (ret) {
        case INTERNAL_ERROR: {
            add_status_line(500, error_500_title);
            add_headers(strlen(error_500_form));
            if (!add_content(error_500_form))
                return false;
            break;
        }
        case BAD_REQUEST: {
            add_status_line(400, error_400_title);
            add_headers(strlen(error_400_form));
            if (!add_content(error_400_form))
                return false;
            break;
        }
        case NO_RESOURCE: {
            add_status_line(404, error_404_title);
            add_headers(strlen(error_404_form));
            if (!add_content(error_404_form))
                return false;
            break;
        }
        case FORBIDDEN_REQUEST: {
            add_status_line(403, error_403_title);
            add_headers(strlen(error_403_form));
            if (!add_content(error_403_form))
                return false;
            break;
        }
        case FILE_REQUEST: {
            add_status_line(200, ok_200_title);
            if (m_file_stat.st_size != 0) {
                add_headers(m_file_stat.st_size);
                m_iv[0].iov_base = m_write_buf;
                m_iv[0].iov_len = m_write_idx;
                m_iv[1].iov_base = m_file_address;
                m_iv[1].iov_len = m_file_stat.st_size;
                m_iv_count = 2;
                bytes_to_send = m_write_idx + m_file_stat.st_size;
                return true;
            } else {
                char *ok_string = "<html><body></body></html>";
                add_headers(strlen(ok_string));
                if (!add_content(ok_string))
                    return false;
            }
        }
        case JSON_REQUEST: {
            add_status_line(200, ok_200_title);
            add_headers(strlen(m_json));
            if (!add_content(m_json)) {
                return false;
            }
            break;
        }
        default: {
            return false;
        }
    }
    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_idx;
    m_iv_count = 1;
    bytes_to_send = m_write_idx;
    return true;
}

/**
 * 处理http报文请求与报文响应
 * 根据read/write的buffer进行报文的解析和响应
 */
void http_conn::process() {
    HTTP_CODE read_ret = process_read();
    if (read_ret == NO_REQUEST) {
        modfd(m_epollfd, m_sockfd, EPOLLIN, m_TRIGMode);
        return;
    }
    bool write_ret = process_write(read_ret);
    if (!write_ret) {
        close_conn();
    }
    modfd(m_epollfd, m_sockfd, EPOLLOUT, m_TRIGMode);
}