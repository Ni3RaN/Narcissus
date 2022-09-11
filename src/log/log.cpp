//
// Created by nie on 22-9-8.
//


#include "log.h"

Log::Log() {
    m_count = 0;
    m_is_async = false;
}

Log::~Log() {
    if (m_fp != nullptr) {
        fclose(m_fp);
    }
}

/**
 * 初始化日志
 * @param file_name 日志文件名
 * @param log_buf_size 缓冲区大小
 * @param split_lines 日志最大行数
 * @param max_queue_size 队列最大容量
 * @return 是否初始化成功
 */
bool Log::init(const char *file_name, int close_log, int log_buf_size, int split_lines, int max_queue_size) {
    //如果设置了异步写日志，就创建阻塞队列
    if (max_queue_size >= 1) {
        m_is_async = true;
        m_log_queue = new block_queue<std::string>(max_queue_size);
        pthread_t tid;
        pthread_create(&tid, nullptr, flush_log_thread, nullptr);
    }
    m_close_log =  close_log;
    m_log_buf_size = log_buf_size;
    m_buf = new char[m_log_buf_size];
    memset(m_buf, '\0', sizeof(m_buf));
    m_split_lines = split_lines;

    time_t t = time(nullptr);
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    const char *p = strrchr(file_name, '/');
    char log_full_name[256] = {'\0'};

    //如果日志文件名不包含路径
    if (p == nullptr) {
        snprintf(log_full_name, 255, "%d_%02d_%02d_%s", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                 file_name);
    } else {
        strcpy(log_name, p + 1);
        strncpy(dir_name, file_name, p - file_name + 1);
        snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1,
                 my_tm.tm_mday, log_name);
    }

    m_today = my_tm.tm_mday;

    m_fp = fopen(log_full_name, "a");

    if (m_fp == nullptr) {
        return false;
    }
    return true;
}

/**
 * 写日志
 * @param level 日志等级
 * @param format 格式化字符串
 * @param ... 可变参数
 */
void Log::write_log(int level, const char *format, ...) {
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    char s[16] = {'\0'};
    //根据日志等级，设置日志前缀
    switch (level) {
        case 0:
            strcpy(s, "[debug]:");
            break;
        case 1:
            strcpy(s, "[info]:");
            break;
        case 2:
            strcpy(s, "[warn]:");
            break;
        case 3:
            strcpy(s, "[error]:");
            break;
        default:
            strcpy(s, "[info]:");
            break;
    }
    //写入一个log，行数加一
    m_mutex.lock();
    m_count++;

    //如果是新的一天或者超出最大行数，就重新生成一个日志文件
    if (m_today != my_tm.tm_mday || m_count % m_split_lines == 0) {
        char new_log[256] = {'\0'};
        fflush(m_fp);
        fclose(m_fp);
        char tail[16] = {'\0'};

        snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);

        if (m_today != my_tm.tm_mday) {
            snprintf(new_log, 255, "%s%s%s", dir_name, tail, log_name);
            m_today = my_tm.tm_mday;
            m_count = 0;
        } else {
            snprintf("new_log", 255, "%s%s%s.%lld", dir_name, tail, log_name, m_count / m_split_lines);
        }
        m_fp = fopen(new_log, "a");
    }

    m_mutex.unlock();

    va_list vaList;
    va_start(vaList, format);

    std::string log_str;
    m_mutex.lock();

    int n = snprintf(m_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s",
                     my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                     my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);

    int m = vsnprintf(m_buf + n, m_log_buf_size - 1, format, vaList);

    m_buf[n + m] = '\n';
    m_buf[n + m + 1] = '\0';

    log_str = m_buf;
    m_mutex.unlock();

    //如果是异步写日志，就将日志写入阻塞队列
    if (m_is_async && !m_log_queue->full()) {
        m_log_queue->push(log_str);
    } else {
        m_mutex.lock();
        fputs(log_str.c_str(), m_fp);
        m_mutex.unlock();
    }

    va_end(vaList);
}

void Log::flush() {
    m_mutex.lock();
    //刷新缓冲区
    fflush(m_fp);
    m_mutex.unlock();
}
