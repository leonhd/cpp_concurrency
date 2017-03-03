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

//#define _declcachealign alignas(64)
#define ALIGNED_ELEMENT 1
template<typename T, int32_t BUF_SIZE_LOG2>
class alignas(64) ring_buf_t
{
public:
	typedef	T ele_typ;

	static const int64_t buf_size_ = (1 << BUF_SIZE_LOG2);
	static const int32_t entry_size_ = sizeof(T);

private:
	static const int64_t pos_mask_ = buf_size_ - 1;

	alignas(64) atomic<int64_t> write_pos_;
	alignas(64) atomic<int64_t> next_write_pos_;
	alignas(64) atomic<int64_t> read_pos_;
	alignas(64) atomic<int64_t> next_read_pos_;

#if ALIGNED_ELEMENT
	struct alignas(64) aligned_ele_typ
	{
		ele_typ val_;
	};

	alignas(64) aligned_ele_typ* buf_;
#else
	ele_typ* buf_;
#endif
public:
	ring_buf_t()
	{
		write_pos_ = 0;
		next_write_pos_ = 0;
		read_pos_ = 0;
		next_read_pos_ = 0;

#if ALIGNED_ELEMENT
		buf_ = new aligned_ele_typ[buf_size_];
#else
		buf_ = new ele_typ[buf_size_];
#endif
	}

	~ring_buf_t()
	{
		delete [] buf_;
		buf_ = nullptr;
	}

	void printf(FILE *pf)
	{
		fprintf(pf, "===\nwrite_pos %I64d, next_write_pos %I64d, writable space %I64d\nread_pos %I64d, next_read_pos %I64d, readable space %I64d\n", write_pos_, next_write_pos_, read_pos_ + buf_size_ - next_write_pos_, read_pos_, next_read_pos_, write_pos_ - next_read_pos_);
		// 		fprintf(pf, "channel rw pos:\n");
		// 		for (int32_t i = 0; i < channel_count_; ++i)
		// 		{
		// 			fprintf(pf, "\tread pos %I64d, write pos %I64d\n", channel_pos_array_[i].read_pos_, channel_pos_array_[i].write_pos_);
		// 		}
	}

	ele_typ& get(int64_t pos)
	{
#if ALIGNED_ELEMENT
		return buf_[pos & pos_mask_].val_;
#else
		return buf_[pos & pos_mask_];
#endif
	}

	int64_t prepare_write(int64_t len)
	{
		return next_write_pos_.fetch_add(len, memory_order::memory_order_acq_rel);//_InterlockedExchangeAdd64(&next_write_pos_, len);
	}

	void publish_write(int64_t pos, int64_t len)
	{
		int32_t loop_count = 0;
		//while (_InterlockedCompareExchange64(&write_pos_, pos + len, pos) != pos)
		int64_t pos_expected = pos, pos_to_write = pos + len;
		while (!write_pos_.compare_exchange_strong(pos_expected, pos_to_write, memory_order::memory_order_acq_rel, memory_order::memory_order_acquire))
		{
			pos_expected = pos;
			this_thread::yield();
			++loop_count;
			if (loop_count > 64)
			{
				loop_count -= 64;
				this_thread::sleep_for((chrono::duration<int64_t, ratio<1, 1000000>>)1);
			}
		}
	}

	bool check_writable(int64_t pos, int64_t len)
	{
		return (pos + len <= read_pos_.load(memory_order::memory_order_acquire) + buf_size_);
	}

	int64_t get_writable_count(int64_t pos)
	{
		return (read_pos_.load(memory_order::memory_order_acquire) + buf_size_ - pos);
	}

	int64_t get_writable_count()
	{
		return get_writable_count(next_write_pos_.load(memory_order::memory_order_acquire));
	}

	int64_t prepare_read(int64_t len)
	{
		return next_read_pos_.fetch_add(len, memory_order::memory_order_acq_rel);//_InterlockedExchangeAdd64(&next_read_pos_, len);
	}

	void publish_read(int64_t pos, int64_t len)
	{
		int32_t loop_count = 0;
		int64_t pos_expected = pos, pos_to_write = pos + len;
		//while (_InterlockedCompareExchange64(&read_pos_, pos + len, pos) != pos)
		while (!read_pos_.compare_exchange_strong(pos_expected, pos_to_write, memory_order::memory_order_acq_rel, memory_order::memory_order_acquire))
		{
			pos_expected = pos;
			this_thread::yield();
			++loop_count;
			if (loop_count > 64)
			{
				loop_count -= 64;
				this_thread::sleep_for((chrono::duration<int64_t, ratio<1, 1000000>>)1);
			}
		}
	}

	bool check_readable(int64_t pos, int64_t len)
	{
		return (pos + len <= write_pos_.load(memory_order::memory_order_acquire));
	}

	int64_t get_readable_count(int64_t pos)
	{
		return (write_pos_.load(memory_order::memory_order_acquire) - pos);
	}

	int64_t get_readable_count()
	{
		return get_readable_count(next_read_pos_.load(memory_order::memory_order_acquire));
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

	//test ring buffer based on atomics
	template<typename T>
	static void test_atomics_rb(int32_t count)
	{
		using clock_t = chrono::high_resolution_clock;
		using us_t = chrono::duration<double, std::ratio<1, 1000000>>;
		std::cout << "loop count: " << count << std::endl;

		clock_t::time_point t_start = clock_t::now();
		typedef ring_buf_t<T, 20> rb_t;
		cout << "size of ring buffer is " << sizeof(rb_t) << std::endl;
		rb_t sync_queue;
		T ret2, ret3;
		int32_t t2_loop, t3_loop;
		atomic<bool> signal;
		signal = false;

		thread t3([&] {
			T ret = -1;
			int32_t loops = 0;
			while (!signal.load(memory_order::memory_order_acquire))
			{
				int64_t pos = sync_queue.prepare_read(1);
				while (!sync_queue.check_readable(pos, 1) && !signal.load(memory_order::memory_order_acquire))
					this_thread::yield();

                if (sync_queue.check_readable(pos, 1))
                {
                    ret = sync_queue.get(pos);
                    sync_queue.publish_read(pos, 1);
                }
                //cout << this_thread::get_id() << "\t" << ret << endl;
                ++loops;
			}
            ret3 = ret;
			t3_loop = loops;
		});

		thread t2([&] {
			T ret = -1;
			int32_t loops = 0;
			while (!signal.load(memory_order::memory_order_acquire))
			{
				int64_t pos = sync_queue.prepare_read(1);
				while (!sync_queue.check_readable(pos, 1) && !signal.load(memory_order::memory_order_acquire))
					this_thread::yield();

                if (sync_queue.check_readable(pos, 1))
                {
                    ret = sync_queue.get(pos);
                    sync_queue.publish_read(pos, 1);
				}
				//cout << this_thread::get_id() << "\t" << ret << endl;
				++loops;
			}
			ret2 = ret;
			t2_loop = loops;
		});

		//this_thread::sleep_for((us_t)10000);

		thread t1([&] {
			T val = 0;
			while (val < count)
			{
				int64_t pos = sync_queue.prepare_write(1);

				while (!sync_queue.check_writable(pos, 1))
					this_thread::yield();

				sync_queue.get(pos) = val++;
				sync_queue.publish_write(pos, 1);
			}

            while (sync_queue.get_readable_count() >= 0)
            {
                this_thread::sleep_for((chrono::duration<int64_t, ratio<1, 10000>>)1);
                this_thread::yield();
            }
			signal.store(true, memory_order::memory_order_release);
		});

		thread::id t2_id = t2.get_id(), t3_id = t3.get_id();
		t1.join();
		t2.join();
		t3.join();
		clock_t::time_point t_stop = clock_t::now();
		us_t time_span = chrono::duration_cast<us_t>(t_stop - t_start);
		cout << "last value of thread t2(" << t2_id << "): " << ret2 << " after " << t2_loop << " loops" << endl;
		cout << "last value of thread t3(" << t3_id << "): " << ret3 << " after " << t3_loop << " loops" << endl;
		std::cout << "push/pop " << count << " entries costs " << time_span.count() << "us" << std::endl;
	}
};
