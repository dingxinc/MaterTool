#pragma once
#include "../tool/SingletonMemory.h"

void TestSingleMemory()
{
    std::thread t1([]()
        { std::cout << "thread t1 singleton address is 0x: " << SingleMemoryModel::GetInst() << std::endl; });

    std::thread t2([]()
        { std::cout << "thread t2 singleton address is 0x: " << SingleMemoryModel::GetInst() << std::endl; });

    t2.join();
    t1.join();
}