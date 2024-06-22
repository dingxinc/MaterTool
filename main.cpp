// MaterTool.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "test/test_algorithm.h"

int main()
{
    TestParallelForEach();
    auto nevc = TestAsyncForEach();
    TestParallelFind(nevc);
    AsyncParallelFind(nevc);
    TestParallelPartialSum();
    return 0;
}
