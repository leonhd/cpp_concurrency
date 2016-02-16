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
	bool signaled_;
public:
	atomic_queue_t() : signaled_(false) {};

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
	static void test_atomic_queue(int32_t count)
	{
		atomic_queue_t<T> atomic_queue;		

		thread t1([&]{
			T val = 0;
			for (int32_t i = 0; i < count; ++i)
				atomic_queue.push(val++);

			atomic_queue.signal(true);
		});

		thread t2([&]{
			T ret;
			while (atomic_queue.pop(ret))
			{
				cout << "2 " << ret << endl;
			}
			//cout << ret << endl;
		});

		thread t3([&]{
			T ret;
			while (atomic_queue.pop(ret))
			{
				cout << "3 " << ret << endl;
			}
			//cout << ret << endl;
		});

		t1.join();
		t2.join();
		t3.join();
	}
};