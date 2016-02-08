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
	//sync based on atomics
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

	//prototype rw lock
	static void test2(int32_t limit)
	{
		int g_count = 0;
		atomic<int32_t> lock = 0;//0 - no use; 1 - read; 2 - write
		atomic<int32_t> rc = 0; //read count
		atomic<int32_t> read_end = false;

		thread t1([&]{
			while (!read_end)
			{
				++rc;
				int32_t ref = 2;
				bool succ = false;
				while (!succ && ref == 2)
				{
					ref = 0;
					succ = lock.compare_exchange_strong(ref, 1, memory_order_release, memory_order_acquire);
				}

				cout << "read1 count:" << g_count << endl;

				if (rc == 1)
					lock.store(0, memory_order_release);
				--rc;

				Sleep(1);
			}
		});

		thread t2([&]{
			while (!read_end)
			{
				++rc;
				int32_t ref = 2;
				bool succ = false;
				while (!succ && ref == 2)
				{
					ref = 0;
					succ = lock.compare_exchange_strong(ref, 1, memory_order_release, memory_order_acquire);
				}

				cout << "read2 count:" << g_count << endl;

				if (rc == 1)
					lock.store(0, memory_order_release);
				--rc;

				Sleep(1);
			}
		});

		thread t3([&]{
			int32_t tmp = 0;
			while (tmp < limit)
			{
				int32_t ref = 2;
				bool succ = false;
				while (!succ && ref != 0)
				{
					ref = 0;
					succ = lock.compare_exchange_strong(ref, 2, memory_order_release, memory_order_acquire);
				}

				tmp = ++g_count;
				cout << "write count:" << tmp << endl;

				lock.store(0, memory_order_release);

				Sleep(1);
			}
		});

		t3.join();
		read_end.store(1, memory_order_release);
		t1.join();
		t2.join();
	}
};