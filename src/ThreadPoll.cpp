#include <iostream>
#include <pthread.h>
#include <string.h>
#include <string>
#include <unistd.h>

#include "ThreadPoll.h"

template <typename T>
ThreadPoll<T>::ThreadPoll(int min, int max){
    // 实例化任务队列
    do{
        taskQ = new TaskQueue<T>;
        if (taskQ == nullptr){
            std::cout << "malloc taskQ fail" << std::endl;
            break;
        }

        threadIDs = new pthread_t[max]; // 创建工作线程ID数组
        if (threadIDs == nullptr){
            std::cout << "malloc threadIDs fail" << std::endl;
            break;
        }
        memset(threadIDs, 0, sizeof(pthread_t) * max); // 初始化工作线程ID数组
        minNum = min;                                  // 初始化最小线程数
        maxNum = max;                                  // 初始化最大线程数
        busyNum = 0;                                   // 初始化忙线程数
        liveNum = min;                                 // 初始化存活线程数和最小线程数一致
        exitNum = 0;                                   // 初始化要销毁的线程数

        if (pthread_mutex_init(&(mutexPool), NULL) != 0 ||
            pthread_cond_init(&(notEmpty), NULL) != 0)  // 判断锁和条件变量是否初始化成功
            {
            std::cout << "init the lock or cond fail" << std::endl;
            break;
        }

        shutdown = false; // 初始化是否要销毁线程池

        // 创建线程
        pthread_create(&managerID, NULL, manager, this);        // 创建管理者线程

        for (int i = 0; i < min; ++i) // 按照最小线程数创建工作线程
        {
            pthread_create(&threadIDs[i], NULL, worker, this);  // 创建工作线程
        }
        return;                                                    // 返回线程池地址
    } while (0);

    // 线程池创建失败，释放资源
    if (threadIDs) delete[]threadIDs;                 // 释放工作线程ID数组
    if (taskQ) delete taskQ;                         // 释放任务队列
}

template <typename T>
ThreadPoll<T>::~ThreadPoll(){
    shutdown = true;                         // 关闭线程池
    pthread_join(managerID, NULL);        // 阻塞回收管理者线程

    // 唤醒阻塞的工作线程
    for (int i = 0; i < liveNum; ++i){
        pthread_cond_broadcast(&notEmpty);
    }

    // 释放堆空间
    if (taskQ) delete taskQ;         // 释放任务队列
    if (threadIDs) delete[]threadIDs; // 释放工作线程ID数组

    pthread_mutex_destroy(&mutexPool);  // 销毁锁
    pthread_cond_destroy(&notEmpty);    // 销毁条件变量
}

template <typename T>
void ThreadPoll<T>::addTask(Task<T> task){
    if (shutdown) return;
    
    // 添加任务
    taskQ->addTask(task);

    // 唤醒工作线程
    pthread_cond_signal(&notEmpty);     // 唤醒工作线程
}

template <typename T>
int ThreadPoll<T>::getAliveNum(){
    pthread_mutex_lock(&mutexPool);   // 给线程池加锁
    int aliveNum = this->liveNum;
    pthread_mutex_unlock(&mutexPool); // 给线程池解锁
    return aliveNum;
}

template <typename T>
int ThreadPoll<T>::getBusyNum(){
    pthread_mutex_lock(&mutexPool);   // 给线程池加锁
    int busyNum = this->busyNum;
    pthread_mutex_unlock(&mutexPool); // 给线程池解锁
    return busyNum;
}

template <typename T>
void* ThreadPoll<T>::worker(void *arg){
    ThreadPoll *poll = static_cast<ThreadPoll*>(arg);       // 获取线程池地址

    while (true)
    {
        pthread_mutex_lock(&(poll->mutexPool)); // 给线程池加锁
        // 判断当前任务队列是否为空
        while (poll->taskQ->getTaskNum() == 0 && !poll->shutdown)
        {
            //  阻塞工作线程
            pthread_cond_wait(&(poll->notEmpty), &(poll->mutexPool)); // 阻塞工作线程，等待任务队列不为空
            if (poll->exitNum > 0){
                poll->exitNum--;        // 要销毁的线程数减1
                if (poll->liveNum > poll->minNum){
                    poll->liveNum--;    // 存活线程数减1
                    pthread_mutex_unlock(&(poll->mutexPool));         // 给线程池解锁
                    poll->threadExit();   // 销毁线程
                }
            }
        }

        // 判断线程池是否被关闭了
        if (poll->shutdown)
        {
            pthread_mutex_unlock(&(poll->mutexPool));   // 解锁
            poll->threadExit();                           // 退出线程
        }

        // 从任务队列中取出一个任务
        Task<T> task = poll->taskQ->takeTask();

        poll->busyNum++; // 忙线程数加1
        pthread_mutex_unlock(&(poll->mutexPool));   // 给线程池解锁

        std::cout << "thread" << std::to_string(pthread_self()) << "start working..." << std::endl;    //开始工作

        task.func(task.arg);                    // 执行任务函数
        T* arg = static_cast<T*>(task.arg);
        delete arg;                         //传入堆空间的地址，需要手动释放
        task.arg = nullptr;                            //防止野指针

        std::cout << "thread" << std::to_string(pthread_self()) << "end working..." << std::endl;   //结束工作

        pthread_mutex_lock(&(poll->mutexPool));     // 给busyNum加锁
        poll->busyNum--;                            // 忙线程数减1
        pthread_mutex_unlock(&(poll->mutexPool));   // 给busyNum解锁
    }
    return NULL;
}

template <typename T>
void* ThreadPoll<T>::manager(void* arg){
    ThreadPoll *poll = static_cast<ThreadPoll*>(arg);           // 获取线程池地址
    while (!poll->shutdown)
    {
        // 每隔3s检测一次
        sleep(3);

        // 取出线程池中任务的数量和当前线程的数量
        pthread_mutex_lock(&(poll->mutexPool));     // 给线程池加锁
        int queueSize = poll->taskQ->getTaskNum();            // 获取任务队列中实际任务数
        int liveNum = poll->liveNum;                // 获取存活线程数
        int busyNum = poll->busyNum;                // 获取忙线程数
        pthread_mutex_unlock(&(poll->mutexPool));   // 给线程池解锁

        // 添加线程
        // 任务数 > 存活线程数 && 存活线程数 < 最大线程数
        if (queueSize > liveNum && liveNum < poll->maxNum){
            pthread_mutex_lock(&(poll->mutexPool)); // 给线程池加锁
            int counter = 0;                        // 记录成功创建的线程个数
            for (int i = 0; 
            i < poll->maxNum                        // 线程池中线程的个数 < 最大线程数
            && counter < NUMBER                     // 成功创建的线程个数 < 线程池中线程的个数
            && poll->liveNum < poll->maxNum;        // 存活线程数 < 最大线程数
            ++i){
                if (poll->threadIDs[i] == 0){
                    pthread_create(&poll->threadIDs[i], NULL, worker, poll); // 创建工作线程
                    counter++;                      // 成功创建的线程个数加1
                    liveNum++;                // 存活线程数加1
                }
            }
            pthread_mutex_unlock(&(poll->mutexPool)); // 给线程池解锁
        }

        // 销毁线程
        // 忙线程 * 2 < 存活线程数 && 存活线程数 > 最小线程数
        if (busyNum*2 < liveNum && liveNum > poll->minNum) {
            pthread_mutex_lock(&(poll->mutexPool));     // 给线程池加锁
            poll->exitNum = NUMBER; // 要销毁的线程数 = 2
            pthread_mutex_unlock(&(poll->mutexPool));   // 给线程池解锁
            // 让工作的线程自杀 666
            for (int i = 0; i < NUMBER; ++i){
                pthread_cond_signal(&(poll->notEmpty)); // 唤醒工作线程
            }
        }
    }
    return NULL;
}

template <typename T>
void ThreadPoll<T>::threadExit(){
    pthread_t tid = pthread_self(); // 获取当前线程ID
    for (int i = 0; i < maxNum; ++i)
    {
        // 找到要销毁的线程
        if (threadIDs[i] == tid){
            threadIDs[i] = 0; // 将要销毁的线程ID置为0
            std::cout << "threadExit() called, " << std::to_string(tid) << "exiting..." << std::endl;
            break;
        }
    }
    pthread_exit(NULL);             // 线程自杀
}