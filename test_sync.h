#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <iostream>
#include <chrono>
#include <array>
using namespace std;

template<typename T>
class sync_queue_t
{
	mutable mutex mut_;
	condition_variable cond_;
	queue<T> data_que_;
	bool signaled_;
public:
	sync_queue_t() : signaled_(false) {};

	sync_queue_t(sync_queue_t const& other)
	{
		lock_guard<mutex> lk(other.mut_);
		data_que_ = other.data_que_;
	}

	bool empty() const
	{
		lock_guard<mutex> lk(mut_);
		return data_que_.empty();
	}

	void push(T const & val)
	{
		lock_guard<mutex> lk(mut_);
		data_que_.push(val);
		cond_.notify_one();
	}

	bool pop(T & ret)
	{
		unique_lock<mutex> lk(mut_);
		cond_.wait(lk, [&]{ return !data_que_.empty() || signaled_; });

		bool ret_tag = !data_que_.empty();
		if (ret_tag)
		{
			ret = data_que_.front();
			data_que_.pop();
		}

		return ret_tag;
	}

	bool try_pop(T & ret)
	{
		lock_guard<mutex> lk(mut_);
		if (data_que_.empty())
			return false;

		ret = data_que_.front();
		data_que_.pop();

		return true;
	}

	bool signal(bool new_val)
	{
		bool prev_val = false;
		{
			lock_guard<mutex> lk(mut_);

			prev_val = signaled_;
			if (prev_val != new_val)
				signaled_ = new_val;
		}

		if (!prev_val && new_val)
			cond_.notify_all();

		return prev_val;
	}
};

class sync_tester_t
{
public:

	template<typename T>
	static void test_sync_queue(int32_t count)
	{
		using clock_t = chrono::high_resolution_clock;
		using us_t = chrono::duration<double, std::ratio<1, 1000000>>;
		std::cout << "loop count: " << count << std::endl;

		clock_t::time_point t_start = clock_t::now();
		sync_queue_t<T> sync_queue;
		std::array<T, 3> ret_vals;
		std::array<int32_t, 3> loops;
		std::array<thread::id, 3> tids;

		auto fn_reader = [&](T &ret, int32_t & loops, thread::id & tid)
		{
			tid = this_thread::get_id();
			T ret_val = -1;
			int32_t loop_val = 0;
			while (sync_queue.pop(ret_val))
			{
				//cout << this_thread::get_id() << "\t" << ret << endl;
				++loop_val;
			}
			ret = ret_val;
			loops = loop_val;
		};

		thread t4(std::bind(fn_reader, std::ref<T>(ret_vals[2]), std::ref<int32_t>(loops[2]), std::ref<thread::id>(tids[2])));
		thread t3(std::bind(fn_reader, std::ref<T>(ret_vals[1]), std::ref<int32_t>(loops[1]), std::ref<thread::id>(tids[1])));
		thread t2(std::bind(fn_reader, std::ref<T>(ret_vals[0]), std::ref<int32_t>(loops[0]), std::ref<thread::id>(tids[0])));
		
		thread t1([&]{
			T val = 0;
			for (int32_t i = 0; i < count; ++i)
				sync_queue.push(val++);

			sync_queue.signal(true);
		});
		std::cout << thread::hardware_concurrency() << std::endl;

		t2.join();
		t3.join();
		t4.join();
		t1.join();
		clock_t::time_point t_stop = clock_t::now();
		us_t time_span = chrono::duration_cast<us_t>(t_stop - t_start);
		for (int32_t i = 0; i < tids.size(); ++i)			
			cout << "last value of reader thread " << i << "(" << tids[i] << "): " << ret_vals[i] << " after " << loops[i] << " loops" << endl;
		std::cout << "push/pop " << count << " entries costs " << time_span.count() << "us" << std::endl;
	}
};