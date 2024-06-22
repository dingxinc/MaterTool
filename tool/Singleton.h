#pragma once

// 单例模式封装
// 2024-6-22
#include <memory>
#include <iostream>
#include <mutex>

template <typename T>
class Singleton {
protected:
	Singleton() = default;
	Singleton(const Singleton<T>&) = delete;
	Singleton& operator=(const Singleton<T>&) = delete;

	static std::shared_ptr<T> instance_;

public:
	static std::shared_ptr<T> GetInstance() {
		static std::once_flag s_flag;
		std::call_once(s_flag, [&]() {
			instance_ = std::shared_ptr<T>(new T);
		});

		return instance_;
	}

	void printAddress() {
		std::cout << "this instance address is : " << instance_.get() << std::endl;
	}

	~Singleton() {
		std::cout << "this instance destruct!" << std::endl;
	}
};

template <typename T>
std::shared_ptr<T> Singleton<T>::instance_ = nullptr;