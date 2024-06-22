#pragma once
#include "../container/lock_free_queue.h"
#include <thread>
#include <iostream>

#define TESTCOUNT 10
void TestLockFreeQue() {
	lock_free_queue<int>  que;
	std::thread t1([&]() {
		for (int i = 0; i < TESTCOUNT; i++) {
			que.push(i);
			std::cout << "push data is " << i << std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		});



	std::thread t2([&]() {
		for (int i = 0; i < TESTCOUNT;) {
			auto p = que.pop();
			if (p == nullptr) {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				continue;
			}
			i++;
			std::cout << "pop data is " << *p << std::endl;
		}
		});

	t1.join();
	t2.join();

	assert(que.destruct_count == TESTCOUNT);
}

void TestLockFreeQueBase() {

	lock_free_queue<int>  que;
	std::thread t1([&]() {
		for (int i = 0; i < TESTCOUNT; i++) {
			que.pop();
			que.pop();
			que.push(i);
			std::cout << "push data is " << i << std::endl;
			auto data = que.pop();
			std::cout << "pop data is " << *data << std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		});

	t1.join();
}


void TestLockFreeQueMultiPop() {
	lock_free_queue<int>  que;
	std::thread t1([&]() {
		for (int i = 0; i < TESTCOUNT * 200; i++) {
			que.push(i);
			std::cout << "push data is " << i << std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		});



	std::thread t2([&]() {
		for (int i = 0; i < TESTCOUNT * 100;) {
			auto p = que.pop();
			if (p == nullptr) {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				continue;
			}
			i++;
			std::cout << "pop data is " << *p << std::endl;
		}
		});

	std::thread t3([&]() {
		for (int i = 0; i < TESTCOUNT * 100;) {
			auto p = que.pop();
			if (p == nullptr) {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				continue;
			}
			i++;
			std::cout << "pop data is " << *p << std::endl;
		}
		});

	t1.join();
	t2.join();
	t3.join();

	assert(que.destruct_count == TESTCOUNT * 200);
}


void TestLockFreeQueMultiPushPop() {
	lock_free_queue<int>  que;
	std::thread t1([&]() {
		for (int i = 0; i < TESTCOUNT * 100; i++) {
			que.push(i);
			std::cout << "push data is " << i << std::endl;
			//std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		});

	std::thread t4([&]() {
		for (int i = TESTCOUNT * 100; i < TESTCOUNT * 200; i++) {
			que.push(i);
			std::cout << "push data is " << i << std::endl;
			//std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		});

	std::thread t2([&]() {
		for (int i = 0; i < TESTCOUNT * 100;) {
			auto p = que.pop();
			if (p == nullptr) {
				//std::this_thread::sleep_for(std::chrono::milliseconds(10));
				continue;
			}
			i++;
			std::cout << "pop data is " << *p << std::endl;
		}
		});

	std::thread t3([&]() {
		for (int i = 0; i < TESTCOUNT * 100;) {
			auto p = que.pop();
			if (p == nullptr) {
				//std::this_thread::sleep_for(std::chrono::milliseconds(10));
				continue;
			}
			i++;
			std::cout << "pop data is " << *p << std::endl;
		}
		});

	t1.join();
	t2.join();
	t3.join();
	t4.join();
	assert(que.destruct_count == TESTCOUNT * 200);
}

void TestLockFreeQueMultiPushPop2() {
	lock_free_queue<int>  que;

	std::thread t1([&]() {
		for (int i = 0; i < TESTCOUNT * 100; i++) {
			que.push(i);
			std::cout << "push data is " << i << std::endl;
			// std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		});

	std::thread t2([&]() {
		for (int i = 0; i < TESTCOUNT * 100; i++) {
			que.push(i);
			std::cout << "push data is " << i << std::endl;
			// std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		});

	std::thread t3([&]() {
		for (int i = 0; i < TESTCOUNT * 100; i++) {
			que.push(i);
			std::cout << "push data is " << i << std::endl;
			// std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		});

	std::thread t4([&]() {
		for (int i = 0; i < TESTCOUNT * 100; i++) {
			que.push(i);
			std::cout << "push data is " << i << std::endl;
			// std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		});

	std::thread t5([&]() {
		for (int i = 0; i < TESTCOUNT * 100;) {
			auto p = que.pop();
			if (p == nullptr) {
				//std::this_thread::sleep_for(std::chrono::milliseconds(10));
				continue;
			}
			i++;
			std::cout << "pop data is " << *p << std::endl;
		}
		});

	std::thread t6([&]() {
		for (int i = 0; i < TESTCOUNT * 100;) {
			auto p = que.pop();
			if (p == nullptr) {
				//std::this_thread::sleep_for(std::chrono::milliseconds(10));
				continue;
			}
			i++;
			std::cout << "pop data is " << *p << std::endl;
		}
		});
	std::thread t7([&]() {
		for (int i = 0; i < TESTCOUNT * 100;) {
			auto p = que.pop();
			if (p == nullptr) {
				//std::this_thread::sleep_for(std::chrono::milliseconds(10));
				continue;
			}
			i++;
			std::cout << "pop data is " << *p << std::endl;
		}
		});

	std::thread t8([&]() {
		for (int i = 0; i < TESTCOUNT * 100;) {
			auto p = que.pop();
			if (p == nullptr) {
				//std::this_thread::sleep_for(std::chrono::milliseconds(10));
				continue;
			}

			i++;
			std::cout << "pop data is " << *p << std::endl;
		}
		});
	t1.join();
	t2.join();
	t3.join();
	t4.join();
	t5.join();
	t6.join();
	t7.join();
	t8.join();
	std::cout << "construct count is " << que.construct_count << std::endl;
	std::cout << "destruct count is " << que.destruct_count << std::endl;
	assert(que.destruct_count == TESTCOUNT * 100 * 4);
}