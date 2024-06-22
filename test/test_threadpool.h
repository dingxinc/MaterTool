#pragma once

#include "../thread/threadpool.h"

void test_thread_pool()
{
    int m = 0;
    ThreadPool::GetInstance()->commit([](int& m)
        {
            m = 1024;
            std::cout << "inner set m is " << m << std::endl; }, m);

    std::this_thread::sleep_for(std::chrono::seconds(3)); /* 实际放在主线程 main 函数中 */
    std::cout << "m is " << m << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(3)); /* 实际放在主线程，main 函数中*/
    ThreadPool::GetInstance()->commit([](int& m)
        {
            m = 1024;
            std::cout << "inner set m is " << m << std::endl; }, std::ref(m));
    std::this_thread::sleep_for(std::chrono::seconds(3)); /* 实际放在主线程，main 函数中*/
    std::cout << "m is " << m << std::endl;
}