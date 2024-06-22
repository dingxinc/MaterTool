#pragma once
// 线程池
// 2024-6-22
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <future>
#include <iostream>
#include "../tool/Singleton.h"

class NoneCopy {
public:
	~NoneCopy() {}
	
protected:
	NoneCopy() {}

private:
	NoneCopy(const NoneCopy&) = delete;
	NoneCopy& operator=(const NoneCopy&) = delete;
};

class ThreadPool : public NoneCopy , public Singleton<ThreadPool> {
	friend class Singleton<ThreadPool>;
public:
	~ThreadPool() {
		stop();
	}

	using Task = std::packaged_task<void()>;

	// 局部静态变量的单例模式，在 C++11 后是线程安全的
	/*static ThreadPool& GetInstance() {
		static ThreadPool instance;
		return instance;
	}*/

	/* 任务投递函数 */
	template <typename F, typename... Args> /* F 为回调函数， Args 为回调函数所需要的参数，每个任务的返回类型和所需要的参数不一样，所以使用模板 */
	auto commit(F&& f, Args &&...args) -> std::future<decltype(f(args...))> 
	{/* decltype 推断出 f 函数返回的值的类型并与 future 绑定，通过 .get() 在未来可以获取函数具体返回的值 */
		using RetType = decltype(f(args...));
		if (stop_.load())
		{ /* 如果线程池停止了，直接返回一个空的 std::future 对象 */
			return std::future<RetType>{};
		}
		/* 如果线程池没有停止，会往下走正常逻辑 */
		/* std::packaged_task<RetType()> 这里的 RetType() 相当于函数调用，bind 绑定后生成的是无参的函数返回类型是 RetType，然后调用 RetType() 相当于调用有参的 f(args..) */
		/* packages_task 可以绑定回调函数，当我们执行 task 时，相当于在执行 f(args...) */
		auto task = std::make_shared<std::packaged_task<RetType()>>(   /* f(args..) 的返回类型是 RetType 类型 */
			std::bind(std::forward<F>(f), std::forward<Args>(args)...) /* bind 将函数的参数绑定到函数的内部，生成一个无参函数，新函数的参数是 void */
		);
		std::future<RetType> ret = task->get_future(); /* 获取 f(args...) 任务函数执行后返回的值 */
		{
			std::lock_guard<std::mutex> lk(cv_mtx_);
			/* emplace 中的实际上是一个 task 也是一个构造函数, task 实际上是 packages_task 的智能指针，等价于 std::package_tasks<RetType> task( (*task)() ) 因为 packages_task 的对象可以绑定函数对象 */
			tasks_.emplace([task]
				{ (*task)(); }); /* 将任务投递到任务队列中，当 task 出队的时候，会执行这个 lambda 表达式中指定的回调函数 */
		}
		cv_lock_.notify_one();
		return ret;
	}

	// 获取空闲线程数量
	int idleThreadNum() {
		return thread_num_;
	}

private:
	ThreadPool(unsigned int num = std::thread::hardware_concurrency()) : stop_(false) {
		if (num <= 1)
			thread_num_ = 2;
		else
			thread_num_ = num;

		start();
	}

	/* 线程池启动 */
	void start() {
		for (int i = 0; i < thread_num_; ++i) {
			// emplace_back 会在 vector 的指定位置调用线程的构造函数，线程的构造所需要的参数就是一个 lambda 表达式
			pool_.emplace_back([this]() {
				while (!this->stop_.load()) { // 线程池处于运行态
					Task task;
					{
						std::unique_lock<std::mutex> cv_mt_(cv_mtx_);
						cv_lock_.wait(cv_mt_, [this]() {
							return this->stop_.load() || !this->tasks_.empty();
						});
						if (this->tasks_.empty()) return;   // 任务队列为空

						task = std::move(this->tasks_.front());
						this->tasks_.pop();
					}
					thread_num_--;
					task(); /* 执行任务函数 */
					thread_num_++;
				}
			});
		}
	}

	/* 线程池停止 */
	void stop() {
		stop_.store(true);
		cv_lock_.notify_all();  // 唤醒所有挂起的线程，线程池要退出了
		for (auto& td : pool_) {
			if (td.joinable()) {
				std::cout << "join thread: " << td.get_id() << std::endl;
				td.join();
			}
		}
	}

private:
	std::atomic_int thread_num_;            // 空闲的线程数
	std::atomic_bool stop_;                 // 线程池状态：停止、运行
	std::queue<Task> tasks_;                // 任务队列
	std::vector<std::thread> pool_;         // 线程队列
	std::mutex cv_mtx_;
	std::condition_variable cv_lock_;       // 应使用条件变量而不是循环，防止造成线程的忙等
};