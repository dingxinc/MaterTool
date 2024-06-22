#pragma once
#include "global.h"
#include "../container/thread_safe_list.h"

void TestTailPush()
{
    double_push_list<MyClass> thread_safe_list;
    for (int i = 0; i < 10; i++)
    {

        MyClass mc(i);
        thread_safe_list.push_front(mc);
    }

    thread_safe_list.for_each([](const MyClass& mc)
        { std::cout << "for each print " << mc << std::endl; });

    for (int i = 10; i < 20; i++)
    {
        MyClass mc(i);
        thread_safe_list.push_back(mc);
    }

    thread_safe_list.for_each([](const MyClass& mc)
        { std::cout << "for each print " << mc << std::endl; });

    auto find_data = thread_safe_list.find_first_if([](const MyClass& mc)
        { return (mc.GetData() == 19); });

    if (find_data)
    {
        std::cout << "find_data is " << find_data->GetData() << std::endl;
    }

    thread_safe_list.insert_if([](const MyClass& mc)
        { return (mc.GetData() == 19); }, 20);

    thread_safe_list.for_each([](const MyClass& mc)
        { std::cout << "for each print " << mc << std::endl; });
}

/* ²âÊÔ */
void MultiThreadPush()
{
    double_push_list<MyClass> thread_safe_list;

    std::thread t1([&]()
        {
            for (int i = 0; i < 20000; i++)
            {
                MyClass mc(i);
                thread_safe_list.push_front(mc);
                std::cout << "push front " << i << " success" << std::endl;
            } });

    std::thread t2([&]()
        {
            for (int i = 20000; i < 40000; i++)
            {
                MyClass mc(i);
                thread_safe_list.push_back(mc);
                std::cout << "push back " << i << " success" << std::endl;
            } });

    std::thread t3([&]()
        {
            for (int i = 0; i < 40000; )
            {
                bool rmv_res = thread_safe_list.remove_first([&](const MyClass& mc)
                    {
                        return mc.GetData() == i;
                    });

                if (!rmv_res)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }

                i++;
            } });

    t1.join();
    t2.join();
    t3.join();

    std::cout << "begin for each print...." << std::endl;
    thread_safe_list.for_each([](const MyClass& mc)
        { std::cout << "for each print " << mc << std::endl; });
    std::cout << "end for each print...." << std::endl;
}