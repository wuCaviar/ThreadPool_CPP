#include <unistd.h>
#include <iostream>

// #include "ThreadPool.h"
#include "ThreadPool.cpp"

void taskFunc(void* arg){
    int num = *(int*)arg; // 任务函数中的参数是一个int型的指针，所以要先转换成int型的指针，再取值
    std::cout << "thread" << std::to_string(pthread_self()) << "is working, number = " << std::to_string(num) << std::endl;
    sleep(1); // 休眠1s，模拟任务执行时长
}

int main(){
    // 创建线程池
    ThreadPool<int> pool(3, 10); // 线程池中最小线程数为3，最大线程数为10，队列最大容量为100
    // 添加任务
    for (int i = 0; i < 100; i++){
        int* num = new int(i + 100); // 为任务函数的参数申请空间
        pool.addTask(Task<int>(taskFunc, num)); // 添加任务
    }
    sleep(20); // 等待所有任务完成
    return 0;
}
