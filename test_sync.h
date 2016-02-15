#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <iostream>
using namespace std;

template<typename T>
class atomic_queue_t
{
	mutable mutex mut_;
	condition_variable cond_;
	queue<T> data_que_;
public:
	atomic_queue_t(){};

	atomic_queue_t(atomic_queue_t const& other)
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

	void pop(T & ret)
	{
		unique_lock<mutex> lk(mut_);
		cond_.wait(lk, [&]{ return !data_que_.empty(); });
		ret = data_que_.front();
		data_que_.pop();
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
};

class sync_tester_t
{
public:

	template<typename T>
	static void test_atomic_queue(int32_t count)
	{
		atomic_queue_t<T> atomic_queue;
		atomic<int32_t> end_signal = 0;

		thread t1([&]{
			T val = 0;
			for (int32_t i = 0; i < count; ++i)
				atomic_queue.push(val++);

			end_signal = 1;
		});

		thread t2([&]{
			while (!end_signal || !atomic_queue.empty())
			{
				T ret;
				atomic_queue.pop(ret);
				cout << ret << endl;
			}
		});

		t1.join();
		t2.join();
	}
};