#pragma once

#include <iostream>
#include <vector>
#include "../thread/internal_thread.h"

std::vector<interruptible_thread> background_threads;
std::mutex mtx1;
std::mutex mtx2;
std::condition_variable cv1;
std::condition_variable_any cv2;
void start_background_processing() {
    background_threads.push_back([]() {
        try {
            std::unique_lock<std::mutex> lock(mtx1);
            interruptible_wait(cv1, lock);
        }
        catch (std::exception& ex) {
            std::cout << "catch exception is " << ex.what() << std::endl;
        }

        });

    background_threads.push_back([]() {
        try {
            std::unique_lock<std::mutex> lock(mtx2);
            this_thread_interrupt_flag.wait(cv2, mtx2);
        }
        catch (std::exception& ex) {
            std::cout << "catch exception is " << ex.what() << std::endl;
        }

        });
}