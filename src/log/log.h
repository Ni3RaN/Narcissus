//
// Created by nie on 22-9-8.
//

#ifndef NEREIDS_LOG_H
#define NEREIDS_LOG_H


#include <string>
#include <cstring>
#include <cstdarg>
#include <pthread.h>
#include <cstdarg>
#include "../lock/block_queue.h"

typedef long long ll;

class Log {
public:
    //单例模式
    static Log *get_instance() {
        static Log instance;
        return &instance;
    }

    static void *flush_log_thread(void *args) {
        Log::get_instance()->async_write_log();
    }

    //初始化日志
    bool init(const char *file_name,int close_log, int log_buf_size, int split_lines, int max_queue_size);

    //写日志
    void write_log(int level, const char *format, ...);

    //强制刷新缓冲区
    void flush();

private:
    Log();

    virtual ~Log();

    //异步写日志
    void *async_write_log() {
        std::string single_log;
        while (m_log_queue->pop(single_log)) {
            m_mutex.lock();
            fputs(single_log.c_str(), m_fp);
            m_mutex.unlock();
        }
    }

private:
    char dir_name[128]{}; //日志目录
    char log_name[128]{}; //日志文件名
    int m_split_lines{}; //日志最大行数
    int m_log_buf_size{}; //日志缓冲区大小
    ll m_count; //日志行数
    int m_today{}; //今天日期
    FILE *m_fp{}; //文件指针
    char *m_buf{}; //缓冲区
    block_queue<std::string> *m_log_queue{}; //阻塞队列
    bool m_is_async; //是否同步
    locker m_mutex; //互斥锁
    int m_close_log{}; //是否关闭日志
};

#define LOG_DEBUG(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(0, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_INFO(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(1, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_WARN(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(2, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_ERROR(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(3, format, ##__VA_ARGS__); Log::get_instance()->flush();}


#endif //NEREIDS_LOG_H
