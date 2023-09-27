#include "TaskQueue.h"

TaskQueue::TaskQueue()
{
    pthread_mutex_init(&m_mutex, nullptr); //初始化互斥锁
}

TaskQueue::~TaskQueue()
{
    pthread_mutex_destroy(&m_mutex); //销毁互斥锁
}

void TaskQueue::addTask(Task &task)
{
    pthread_mutex_lock(&m_mutex); //加锁
    m_taskQ.push(task); //添加任务
    pthread_mutex_unlock(&m_mutex); //解锁
}

void TaskQueue::addTask(callback f, void *arg)
{
    pthread_mutex_lock(&m_mutex); //加锁
    m_taskQ.push(Task(f, arg)); //添加任务
    pthread_mutex_unlock(&m_mutex); //解锁
}

Task TaskQueue::takeTask()
{
    Task t;
    pthread_mutex_lock(&m_mutex); //加锁
    if (!m_taskQ.empty()){
        t = m_taskQ.front(); //取出任务
        m_taskQ.pop(); //移除任务
    }
    pthread_mutex_unlock(&m_mutex); //解锁
    return t;
}
