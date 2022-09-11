//
// Created by nie on 22-9-11.
//

#ifndef NEREIDS_THREADPOOL_H
#define NEREIDS_THREADPOOL_H

#include <memory>
#include "../mysql/sql_connection_pool.h"

template<typename T>
class threadpool {
public:
    /**
     * @param actor_model 模型选择
     * @param connPool 数据库连接池
     * @param thread_number 线程池线程数
     * @param max_requests 请求队列最大请求数
     */
    threadpool(int actor_model, std::shared_ptr<sql_connection_pool> connPool, int thread_number = 8,
               int max_requests = 10000) {
        if (thread_number <= 0 || max_requests <= 0) {
            throw std::exception();
        }
        m_threads = new pthread_t[thread_number];
        if (!m_threads) {
            throw std::exception();
        }
        for (int i = 0; i < thread_number; i++) {
            if (pthread_create(m_threads + i, NULL, worker, this) != 0) {
                delete[] m_threads;
                throw std::exception();
            }
            if (pthread_detach(m_threads[i])) {
                delete[] m_threads;
                throw std::exception();
            }
        }
    }

    ~threadpool() {
        delete[] m_threads;
    }

    /**
     * 往请求队列中添加任务
     * @param request 任务
     * @param state 状态
     * @return 是否添加成功,成功返回true,失败返回false
     */
    bool append(std::shared_ptr<T> request, int state) {
        m_queuelocker.lock();
        if (m_workqueue.size() > m_max_requests) {
            m_queuelocker.unlock();
            return false;
        }
        request->m_state = state;
        m_workqueue.push_back(request);
        m_queuelocker.unlock();
        m_queuestat.post();
        return true;
    }

    /**
     * 线程池中的工作线程运行的函数,它不断从工作队列中取出任务并执行之
     * @param request 任务
     * @return 是否执行成功,成功返回true,失败返回false
     */
    bool append_p(std::shared_ptr<T> request) {
        m_queuelocker.lock();
        if (m_workqueue.size() > m_max_requests) {
            m_queuelocker.unlock();
            return false;
        }
        m_workqueue.push_back(request);
        m_queuelocker.unlock();
        m_queuestat.post();
        return true;
    }

private:
    /**
     * 工作线程运行的函数，它不断从工作队列中取出任务并执行之
     */
    static std::shared_ptr<void> worker(void *arg);

    void run();

private:
    int m_thread_number;        //线程池中的线程数
    int m_max_requests;         //请求队列中允许的最大请求数
    pthread_t *m_threads;       //描述线程池的数组，其大小为m_thread_number
    std::list<std::shared_ptr<T> > m_workqueue; //请求队列
    locker m_queuelocker;       //保护请求队列的互斥锁
    sem m_queuestat;            //是否有任务需要处理
    std::shared_ptr<sql_connection_pool> m_connPool;  //数据库连接池
    int m_actor_model;          //模型切换（这个切换是指Reactor/Proactor）
};

template<typename T>
std::shared_ptr<void> threadpool<T>::worker(void *arg) {
    std::shared_ptr<threadpool> pool = (std::shared_ptr<threadpool>) arg;
    pool->run();
    return pool;
}

template<typename T>
void threadpool<T>::run() {
    while (true) {
        m_queuestat.wait();
        m_queuelocker.lock();
        if (m_workqueue.empty()) {
            m_queuelocker.unlock();
            continue;
        }
        std::shared_ptr<T> request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if (!request) {
            continue;
        }
        if (1 == m_actor_model) {
            if (0 == request->m_state) {
                if (request->read_once()) {
                    request->improv = 1;
                    connectionRAII mysqlcon(&request->mysql, m_connPool);
                    request->process();
                } else {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            } else {
                if (request->write()) {
                    request->improv = 1;
                } else {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
        } else {
            connectionRAII mysqlcon(&request->mysql, m_connPool);
            request->process();
        }
    }
}


#endif //NEREIDS_THREADPOOL_H
