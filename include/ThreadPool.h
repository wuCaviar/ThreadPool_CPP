#pragma once

#include "TaskQueue.h"
#include "src/TaskQueue.cpp"

template <typename T>
class ThreadPool
{
public:
    // 构造函数
    ThreadPool(int min, int max);
    // 析构函数
    ~ThreadPool();

    // 添加任务
    void addTask(Task<T> task);
    void addTask(callback f, void* arg);

    // 获取线程池中忙线程的数量
    int getBusyNum();
    // 获取线程池中活着的线程的数量
    int getAliveNum();

private:
    static void* worker(void* arg);        //线程池中工作线程
    static void* manager(void* arg);                 //线程管理者线程
    void threadExit();              //线程退出

private:
    TaskQueue<T>* taskQ;               //任务队列

    pthread_t managerID;            //管理者线程ID
    pthread_t* threadIDs;           //工作线程ID

    int minNum;                     //最小线程数
    int maxNum;                     //最大线程数
    int busyNum;                    //忙线程数
    int liveNum;                    //存活线程数
    int exitNum;                    //要销毁的线程数

    pthread_mutex_t mutexPool;      //锁整个线程池
    pthread_cond_t notEmpty;        //任务队列空，取任务的线程阻塞

    static const int NUMBER = 2;    //每次创建和销毁线程的个数

    bool shutdown;                   //是否要销毁线程池 1:销毁 0:不销毁    
};

