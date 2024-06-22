#pragma once
#include "../container/thread_safe_stack.h"
#include "global.h"

std::mutex mtx_cout;
void PrintMyClass(std::string consumer, std::shared_ptr<MyClass> data)
{
	std::lock_guard<std::mutex> lock(mtx_cout);
	std::cout << consumer << " pop data success , data is " << (*data) << std::endl;
}

void TestThreadSafeStack()
{
	threadsafe_stack_waitable<MyClass> stack;

	std::thread consumer1(
		[&]()
		{
			for (;;)
			{
				std::shared_ptr<MyClass> data = stack.wait_and_pop();
				PrintMyClass("consumer1", data);
			}
		}
	);

	std::thread consumer2([&]()
		{
			for (;;)
			{
				std::shared_ptr<MyClass> data = stack.wait_and_pop();
				PrintMyClass("consumer2", data);
			}
		});

	std::thread producer([&]()
		{
			for (int i = 0; i < 100; i++)
			{
				MyClass mc(i);
				stack.push(std::move(mc));
			}
		});

	consumer1.join();
	consumer2.join();
	producer.join();
}