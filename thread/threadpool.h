#pragma once
// �̳߳�
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

	// �ֲ���̬�����ĵ���ģʽ���� C++11 �����̰߳�ȫ��
	/*static ThreadPool& GetInstance() {
		static ThreadPool instance;
		return instance;
	}*/

	/* ����Ͷ�ݺ��� */
	template <typename F, typename... Args> /* F Ϊ�ص������� Args Ϊ�ص���������Ҫ�Ĳ�����ÿ������ķ������ͺ�����Ҫ�Ĳ�����һ��������ʹ��ģ�� */
	auto commit(F&& f, Args &&...args) -> std::future<decltype(f(args...))> 
	{/* decltype �ƶϳ� f �������ص�ֵ�����Ͳ��� future �󶨣�ͨ�� .get() ��δ�����Ի�ȡ�������巵�ص�ֵ */
		using RetType = decltype(f(args...));
		if (stop_.load())
		{ /* ����̳߳�ֹͣ�ˣ�ֱ�ӷ���һ���յ� std::future ���� */
			return std::future<RetType>{};
		}
		/* ����̳߳�û��ֹͣ���������������߼� */
		/* std::packaged_task<RetType()> ����� RetType() �൱�ں������ã�bind �󶨺����ɵ����޲εĺ������������� RetType��Ȼ����� RetType() �൱�ڵ����вε� f(args..) */
		/* packages_task ���԰󶨻ص�������������ִ�� task ʱ���൱����ִ�� f(args...) */
		auto task = std::make_shared<std::packaged_task<RetType()>>(   /* f(args..) �ķ��������� RetType ���� */
			std::bind(std::forward<F>(f), std::forward<Args>(args)...) /* bind �������Ĳ����󶨵��������ڲ�������һ���޲κ������º����Ĳ����� void */
		);
		std::future<RetType> ret = task->get_future(); /* ��ȡ f(args...) ������ִ�к󷵻ص�ֵ */
		{
			std::lock_guard<std::mutex> lk(cv_mtx_);
			/* emplace �е�ʵ������һ�� task Ҳ��һ�����캯��, task ʵ������ packages_task ������ָ�룬�ȼ��� std::package_tasks<RetType> task( (*task)() ) ��Ϊ packages_task �Ķ�����԰󶨺������� */
			tasks_.emplace([task]
				{ (*task)(); }); /* ������Ͷ�ݵ���������У��� task ���ӵ�ʱ�򣬻�ִ����� lambda ���ʽ��ָ���Ļص����� */
		}
		cv_lock_.notify_one();
		return ret;
	}

	// ��ȡ�����߳�����
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

	/* �̳߳����� */
	void start() {
		for (int i = 0; i < thread_num_; ++i) {
			// emplace_back ���� vector ��ָ��λ�õ����̵߳Ĺ��캯�����̵߳Ĺ�������Ҫ�Ĳ�������һ�� lambda ���ʽ
			pool_.emplace_back([this]() {
				while (!this->stop_.load()) { // �̳߳ش�������̬
					Task task;
					{
						std::unique_lock<std::mutex> cv_mt_(cv_mtx_);
						cv_lock_.wait(cv_mt_, [this]() {
							return this->stop_.load() || !this->tasks_.empty();
						});
						if (this->tasks_.empty()) return;   // �������Ϊ��

						task = std::move(this->tasks_.front());
						this->tasks_.pop();
					}
					thread_num_--;
					task(); /* ִ�������� */
					thread_num_++;
				}
			});
		}
	}

	/* �̳߳�ֹͣ */
	void stop() {
		stop_.store(true);
		cv_lock_.notify_all();  // �������й�����̣߳��̳߳�Ҫ�˳���
		for (auto& td : pool_) {
			if (td.joinable()) {
				std::cout << "join thread: " << td.get_id() << std::endl;
				td.join();
			}
		}
	}

private:
	std::atomic_int thread_num_;            // ���е��߳���
	std::atomic_bool stop_;                 // �̳߳�״̬��ֹͣ������
	std::queue<Task> tasks_;                // �������
	std::vector<std::thread> pool_;         // �̶߳���
	std::mutex cv_mtx_;
	std::condition_variable cv_lock_;       // Ӧʹ����������������ѭ������ֹ����̵߳�æ��
};