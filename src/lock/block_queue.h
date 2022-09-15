//
// Created by nie on 22-9-8.
//

#ifndef NEREIDS_BLOCK_QUEUE_H
#define NEREIDS_BLOCK_QUEUE_H

#include <cstdlib>
#include <pthread.h>
#include <ctime>
#include "locker.h"
template<class T>

/**
 * 循环数组实现的阻塞队列，m_back = (m_back + 1) % m_max_size;
 * 线程安全，每个操作前都要先加互斥锁，操作完后，再解锁
 * @tparam T
 */
class block_queue {
public:

    /**
     * 构造函数
     * @param max_size
     */
    explicit block_queue(int max_size = 1000) {
        if (max_size <= 0) {
            exit(-1);
        }
        m_max_size = max_size;
        m_array = new T[max_size];
        m_size = 0;
        m_front = -1;
        m_back = -1;
    }

    /**
     * 清空队列
     */
    void clear() {
        m_locker.lock();
        m_size = 0;
        m_front = -1;
        m_back = -1;
        m_locker.unlock();
    }

    /**
     * 析构函数
     */
    ~block_queue() {
        m_locker.lock();
        delete[] m_array;
        m_locker.unlock();
    }

    /**
     * 判断队列是否满了
     * @return size >= max_size
     */
    bool full() {
        m_locker.lock();
        if (m_size >= m_max_size) {
            m_locker.unlock();
            return true;
        }
        m_locker.unlock();
        return false;
    }

    /**
     * 判断队列是否为空
     * @return size == 0
     */
    bool empty() {
        m_locker.lock();
        if (m_size == 0) {
            m_locker.unlock();
            return true;
        }
        m_locker.unlock();
        return false;
    }

    /**
     * 返回队首元素
     * @return success: 队首元素，fail: NULL
     */
    bool front(T &value) {
        m_locker.lock();
        if (m_size == 0) {
            m_locker.unlock();
            return false;
        }
        value = m_array[m_front];
        m_locker.unlock();
        return true;
    }

    /**
     * 返回队尾元素
     * @param value
     * @return success: 队尾元素，fail: NULL
     */
    bool back(T &value) {
        m_locker.lock();
        if (m_size == 0) {
            m_locker.unlock();
            return false;
        }
        value = m_array[m_back];
        m_locker.unlock();
        return true;
    }

    /**
     * 返回队列大小
     * @return size
     */
    int size() {
        m_locker.lock();
        int tmp = m_size;
        m_locker.unlock();
        return tmp;
    }

    /**
     * 返回队列最大容量
     * @return max_size
     */
    int max_size() {
        m_locker.lock();
        int tmp = m_max_size;
        m_locker.unlock();
        return tmp;
    }

    /**
     * 向队列中添加元素
     * @param item
     * @return success: true, fail: false
     */
    bool push(const T &item) {
        m_locker.lock();
        if (m_size >= m_max_size) {
            m_cond.broadcast();
            m_locker.unlock();
            return false;
        }
        m_back = (m_back + 1) % m_max_size;
        m_array[m_back] = item;
        m_size++;
        m_cond.broadcast();
        m_locker.unlock();
        return true;
    }

    /**
     * 从队列中取出元素
     * @param item
     * @return success: true, fail: false
     */
    bool pop(T &item) {
        m_locker.lock();
        while (m_size <= 0) {
            if (!m_cond.wait(m_locker.get())) {
                m_locker.unlock();
                return false;
            }
        }
        m_front = (m_front + 1) % m_max_size;
        item = m_array[m_front];
        m_size--;
        m_locker.unlock();
        return true;
    }

    // 超时处理
    bool pop(T &item, int ms_timeout) {
        struct timespec t = {0, 0};
        struct timeval now = {0, 0};
        gettimeofday(&now, nullptr);
        m_locker.lock();
        if (m_size <= 0) {
            t.tv_sec = now.tv_sec + ms_timeout / 1000;
            t.tv_nsec = (ms_timeout % 1000) * 1000;
            if (!m_cond.timewait(m_locker.get(), t)) {
                m_locker.unlock();
                return false;
            }
        }
    }

private:
    locker m_locker;
    cond m_cond;

    int m_max_size;
    T *m_array;
    int m_size;
    int m_front;
    int m_back;

};

#endif //NEREIDS_BLOCK_QUEUE_H
