#pragma once
#include "../algorithm/parallel_algorithm.h"
#include <iostream>

void TestParallelForEach() {
	std::vector<int> nevc;
	for (int i = 0; i < 26; ++i) {
		nevc.push_back(i);
	}

	parallel_for_each(nevc.begin(), nevc.end(), [](int& i) {
		i *= i;
	});

	for (int i = 0; i < nevc.size(); ++i) {
		std::cout << nevc[i] << " ";
	}
	std::cout << std::endl;
}

std::vector<int> TestAsyncForEach() {
	std::vector<int> nevc;
	for (int i = 0; i < 55; ++i) {
		nevc.push_back(i);
	}

	async_for_each(nevc.begin(), nevc.end(), [](int& i) {
		i *= i;
	});

	for (int i = 0; i < nevc.size(); ++i) {
		std::cout << nevc[i] << " ";
	}
	std::cout << std::endl;
	return nevc;
}

void TestParallelFind(std::vector<int>& nevc) {
	auto iter = parallel_find(nevc.begin(), nevc.end(), 25 * 25);
	if (iter == nevc.end()) {
		std::cout << "not found value sqrt 25" << std::endl;
	}
	std::cout << "found value sqrt 25" << *iter << std::endl;
}

void AsyncParallelFind(std::vector<int>& nevc) {
	auto iter = parallel_find_async(nevc.begin(), nevc.end(), 25 * 25);
	if (iter == nevc.end()) {
		std::cout << "not found value sqrt 25" << std::endl;
	}
	std::cout << "found value sqrt 25" << *iter << std::endl;
}

void TestParallelPartialSum() {
	std::vector<int> nevc{ 1,2,3,4,5,6,7,8,9 };
	parallel_partial_sum(nevc.begin(), nevc.end());
	for (int i = 0; i < nevc.size(); ++i) {
		std::cout << nevc[i] << " ";
	}
	std::cout << std::endl;
}