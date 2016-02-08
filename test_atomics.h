#pragma once
#include <thread>
#include <atomic>
#include <stdint.h>
#include <windows.h>
#include <iostream>
using namespace std;

class atomics_tester_t
{
public:
	static void test1(int32_t time_span_ms)
	{
		atomic<int32_t> status = 0;

		thread t1([&]{
			int32_t unit = 100;
			int32_t count = 0;
			while (status.load(memory_order_acquire) == 0)
			{
				::Sleep(unit);
				cout << count++ << endl;
			}

			status += 1;
		});

		int32_t count = 0;
		int32_t unit = 1;
		int32_t limit = (time_span_ms + unit - 1) / unit;
		while (count < limit && status.load(memory_order_acquire) == 0)
		{
			::Sleep(unit);
			++count;
		}
		status += 1;
		t1.join();

		cout << "status is " << status << endl;
	}
};