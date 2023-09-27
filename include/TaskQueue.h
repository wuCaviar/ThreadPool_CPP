#pragma once
#include <queue>
#include <pthread.h>

using callback = void(*)(void* arg); //回调函数指针

//任务结构体
struct Task
{
    Task(){
        func = nullptr;
        arg = nullptr;
    } //默认构造函数

    Task(callback f, void* arg){
        func = f;
        this->arg = arg;
    } //构造函数

    callback func; //回调函数
    void* arg; //回调函数参数
};

//任务队列
class TaskQueue
{
public:
    TaskQueue();
    ~TaskQueue();

    // 添加任务
    void addTask(Task& task);
    void addTask(callback f, void* arg);

    // 取出任务
    Task takeTask();

    // 获取任务数量
    inline int getTaskNum(){
        return m_taskQ.size();
    }

private:
    pthread_mutex_t m_mutex;
    std::queue<Task> m_taskQ;
};