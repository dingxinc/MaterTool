#pragma once
#include "../tool/async_log.h"
#include <chrono>

void TestAsyncLog() {
	AsyncLog::ELog("hello {}", "world!");
	std::this_thread::sleep_for(std::chrono::seconds(2));
}