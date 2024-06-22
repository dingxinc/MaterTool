#pragma once
#include "../container/thread_safe_queue.h"
#include <thread>
#include <chrono>
#include <iostream>

void test_safe_queue()
{
    threadsafe_queue<int> safe_queue;
    std::mutex mtx_print;
    std::thread producer(
        [&]()
        {
            for (int i = 0;; i++)
            {
                safe_queue.push(i);
                {
                    std::lock_guard<std::mutex> lk(mtx_print);
                    std::cout << "producer push data is " << i << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        });

    std::thread consumer1(
        [&]()
        {
            for (;;)
            {
                auto data = safe_queue.wait_and_pop();
                {
                    std::lock_guard<std::mutex> lk(mtx_print);
                    std::cout << "consumer1 pop data is " << *data << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        });

    std::thread consumer2(
        [&]()
        {
            for (;;)
            {
                auto data = safe_queue.try_pop();
                if (data != nullptr)
                {
                    {
                        std::lock_guard<std::mutex> lk(mtx_print);
                        std::cout << "consumer2 pop data is " << *data << std::endl;
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        });

    producer.join();
    consumer1.join();
    consumer2.join();
}