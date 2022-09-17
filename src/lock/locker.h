//
// Created by nie on 22-9-8.
//

#ifndef NEREIDS_LOCKER_H
#define NEREIDS_LOCKER_H

#include <exception>
#include <semaphore.h>
#include <sys/time.h>
#include <pthread.h>

//互斥锁
class locker {
public:
    locker() {
        //构造函数没有返回值，可以通过抛出异常来报告错误
        if (pthread_mutex_init(&m_mutex, nullptr) != 0) {
            throw std::exception();
        }
    }

    ~locker() {
        //构造函数没有返回值，可以通过抛出异常来报告错误
        pthread_mutex_destroy(&m_mutex);
    }

    //互斥锁
    bool lock() {
        return pthread_mutex_lock(&m_mutex) == 0;
    }

    //释放互斥锁
    bool unlock() {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }

    //获取互斥锁
    pthread_mutex_t *get() {
        return &m_mutex;
    }

private:
    pthread_mutex_t m_mutex{};
};

//信号量
class sem {
public:
    sem() {
        //构造函数没有返回值，可以通过抛出异常来报告错误
        if (sem_init(&m_sem, 0, 0) != 0) {
            throw std::exception();
        }
    }

    explicit sem(int num) {
        //构造函数没有返回值，可以通过抛出异常来报告错误
        if (sem_init(&m_sem, 0, num) != 0) {
            throw std::exception();
        }
    }

    ~sem() {
        sem_destroy(&m_sem);
    }

    //等待信号量
    bool wait() {
        return sem_wait(&m_sem) == 0;
    }

    //增加信号量
    bool post() {
        return sem_post(&m_sem) == 0;
    }

private:
    sem_t m_sem{};
};

//条件变量
class cond {
public:
    cond() {
//        if (pthread_mutex_init(&m_mutex, nullptr) != 0) {
//            //构造函数没有返回值，可以通过抛出异常来报告错误
//            throw std::exception();
//        }
        if (pthread_cond_init(&m_cond, nullptr) != 0) {
            //构造函数没有返回值，可以通过抛出异常来报告错误
//            pthread_mutex_destroy(&m_mutex);
            throw std::exception();
        }
    }

    ~cond() {
//        pthread_mutex_destroy(&m_mutex);
        pthread_cond_destroy(&m_cond);
    }

    //等待条件变量
    bool wait(pthread_mutex_t *m_mutex) {
        int ret = 0;
//        pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_wait(&m_cond, m_mutex);
//        pthread_mutex_unlock(&m_mutex);
        return ret == 0;
    }

    //唤醒等待条件变量的线程
    bool timewait(pthread_mutex_t *m_mutex,struct timespec t) {
        int ret = 0;
//        pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_timedwait(&m_cond, m_mutex, &t);
//        pthread_mutex_unlock(&m_mutex);
        return ret == 0;
    }

    //唤醒等待条件变量的线程
    bool signal() {
        return pthread_cond_signal(&m_cond) == 0;
    }

    //唤醒所有等待条件变量的线程
    bool broadcast() {
        return pthread_cond_broadcast(&m_cond) == 0;
    }

private:
//    pthread_mutex_t m_mutex{};
    pthread_cond_t m_cond{};
};

#endif //NEREIDS_LOCKER_H
