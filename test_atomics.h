#pragma once
#include <thread>
#include <atomic>
#include <stdint.h>
//#include <windows.h>
#include <thread>
#include <iostream>
using namespace std;

template<typename T>
class rw_lock_t
{
public:
	typedef T typ;
private:
	atomic<typ> read_count_;
	atomic<typ> lock_;
public:
	rw_lock_t() : read_count_(0), lock_(0){};

	void acquire_read()
	{
		++read_count_;
		int32_t ref = 2;
		bool succ = false;
		while (!succ && ref == 2)
		{
			ref = 0;
			succ = lock_.compare_exchange_strong(ref, 1, memory_order_release, memory_order_acquire);
		}
	}

	void release_read()
	{
		if (read_count_ == 1)
			lock_.store(0, memory_order_release);
		--read_count_;
	}

	void acquire_write()
	{
		int32_t ref = 2;
		bool succ = false;
		while (!succ)
		{
			ref = 0;
			succ = lock_.compare_exchange_strong(ref, 2, memory_order_release, memory_order_acquire);
		}
	}

	void release_write()
	{
		lock_.store(0, memory_order_release);
	}
};

class atomics_tester_t
{
public:
	//sync based on atomics
	static void test1(int32_t time_span_ms)
	{
		atomic<int32_t> status;
		status = 0;

		thread t1([&]{
			int32_t unit = 100;
			int32_t count = 0;
			while (status.load(memory_order_acquire) == 0)
			{
                this_thread::sleep_for((chrono::duration<double, ratio<1, 1>>)(unit / 1000.0));
                cout << count++ << endl;
			}

			status += 1;
		});

		int32_t count = 0;
		int32_t unit = 1;
		int32_t limit = (time_span_ms + unit - 1) / unit;
		while (count < limit && status.load(memory_order_acquire) == 0)
		{
			this_thread::sleep_for((chrono::duration<double, ratio<1, 1>>)(unit / 1000.0));
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
		rw_lock_t<int32_t> lock;
		atomic<int32_t> read_end;
		read_end = false;

		thread t1([&]{
			while (!read_end)
			{
				lock.acquire_read();

				cout << "read1 count:" << g_count << endl;

				lock.release_read();

				this_thread::sleep_for((chrono::duration<double, ratio<1, 1>>)(1 / 1000.0));;
			}
		});

		thread t2([&]{
			while (!read_end)
			{
				lock.acquire_read();

				cout << "read2 count:" << g_count << endl;

				lock.release_read();

				this_thread::sleep_for((chrono::duration<double, ratio<1, 1>>)(1 / 1000.0));;
			}
		});

		thread t3([&]{
			int32_t tmp = 0;
			while (tmp < limit)
			{
				lock.acquire_write();

				tmp = ++g_count;
				cout << "write count:" << tmp << endl;

				lock.release_write();

				this_thread::sleep_for((chrono::duration<double, ratio<1, 1>>)(1 / 1000.0));
			}
		});

		t3.join();
		read_end.store(1, memory_order_release);
		t1.join();
		t2.join();
	}
};
