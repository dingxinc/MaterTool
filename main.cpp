// MaterTool.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "test/test_algorithm.h"
#include "test/test_threadpool.h"
#include "test/test_interupthread.h"
#include "test/test_lockfreequeue.h"

int main()
{
    /*TestParallelForEach();
    auto nevc = TestAsyncForEach();
    TestParallelFind(nevc);
    AsyncParallelFind(nevc);
    TestParallelPartialSum();*/

    // test_thread_pool();

    /*start_background_processing();
    for (unsigned i = 0; i < background_threads.size(); i++) {
        background_threads[i].interrupt();
    }

    for (unsigned i = 0; i < background_threads.size(); i++) {
        background_threads[i].join();
    }*/

    TestLockFreeQueMultiPushPop2();
    return 0;
}
