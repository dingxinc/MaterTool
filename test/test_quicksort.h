#pragma once
#include "../algorithm/quick_sort.h"

// 测试递归快排
void test_quick_sort()
{
    int num_arr[] = { 5, 3, 7, 6, 4, 1, 0, 2, 9, 10, 8 };
    int length = sizeof(num_arr) / sizeof(int);
    quick_sort(num_arr, length);
    std::cout << "sorted result is ";
    for (int i = 0; i < length; i++)
    {
        std::cout << " " << num_arr[i];
    }

    std::cout << std::endl;
}

// 测试函数式快排
void test_sequential_quick()
{
    std::list<int> numlists = { 6, 1, 0, 7, 5, 2, 9, -1 };
    auto sort_result = sequential_quick_sort(numlists);
    std::cout << "sorted result is ";
    for (auto iter = sort_result.begin(); iter != sort_result.end(); iter++)
    {
        std::cout << " " << (*iter);
    }
    std::cout << std::endl;
}

// 测试并行快排
void test_parallel_quick_sort()
{
    std::list<int> numlists = { 6, 1, 0, 7, 5, 2, 9, -1 };
    auto sort_result = parallel_quick_sort(numlists);
    std::cout << "sorted result is ";
    for (auto iter = sort_result.begin(); iter != sort_result.end(); iter++)
    {
        std::cout << " " << (*iter);
    }
    std::cout << std::endl;
}

// 测试线程池快排
void test_thread_pool_sort()
{
    std::list<int> numlists = { 6, 1, 0, 7, 5, 2, 9, -1 };
    auto sort_result = thread_pool_quick_sort(numlists);
    std::cout << "sorted result is ";
    for (auto iter = sort_result.begin(); iter != sort_result.end(); iter++)
    {
        std::cout << " " << (*iter);
    }
    std::cout << std::endl;
}