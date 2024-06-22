#pragma once
// Ïß³ÌÍË³ö
// 2024-6-22

#include <thread>
#include <vector>

class join_threads {
public:
	explicit join_threads(std::vector<std::thread>& threads) : threads_(threads) {}
	~join_threads() {
		for (unsigned long i = 0; i < threads_.size(); ++i) {
			if (threads_[i].joinable()) {
				threads_[i].join();
			}
		}
	}

private:
	std::vector<std::thread>& threads_;
};