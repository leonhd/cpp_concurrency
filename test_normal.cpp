#include "test_normal.h"
#include <stdint.h>
#include <chrono>
#include <iostream>
#include <atomic>
using namespace std;

struct val_interface
{
	virtual int32_t val() = 0;
};

struct val_t : public val_interface {
	int32_t val_;

public:
	val_t(int32_t val)
	{
		val_ = val;
	}

	virtual int32_t val()
	{
		return val_;
	}

	int32_t val1()
	{
		return val_;
	}
};

volatile int32_t ival = 0;
atomic<int32_t> a_ival = 0;
void normal_tester_t::test_mem_access(int32_t count)
{
	using clock_t = chrono::high_resolution_clock;
	using us_t = chrono::duration<double, std::ratio<1, 1000000>>;
	std::cout << "loop count " << count << std::endl;

	val_t val(0);
	val_interface *val_ptr = &val;
	val_t *val_ptr1 = &val;
	val_t &val_ref = val;

	//virtual method on ref
	{
		clock_t::time_point t0 = clock_t::now();
		int32_t sum = 0;
		for (int32_t i = 0; i < count; ++i)
			sum += val_ref.val();
		clock_t::time_point t1 = clock_t::now();

		us_t time_span = chrono::duration_cast<us_t>(t1 - t0);
		std::cout << "\tvirtual method on ref: it takes " << time_span.count() << " us to get the result " << sum << std::endl;
	}

	//virtual method on concrete ptr
	{
		clock_t::time_point t0 = clock_t::now();
		int32_t sum = 0;
		for (int32_t i = 0; i < count; ++i)
			sum += val_ptr1->val();
		clock_t::time_point t1 = clock_t::now();

		us_t time_span = chrono::duration_cast<us_t>(t1 - t0);
		std::cout << "\tvirtual method on concreate ptr: it takes " << time_span.count() << " us to get the result " << sum << std::endl;
	}

	//virtual method on abstract ptr
	{
		clock_t::time_point t0 = clock_t::now();
		int32_t sum = 0;
		for (int32_t i = 0; i < count; ++i)
			sum += val_ptr->val();
		clock_t::time_point t1 = clock_t::now();

		us_t time_span = chrono::duration_cast<us_t>(t1 - t0);
		std::cout << "\tvirtual method on abstract ptr: it takes " << time_span.count() << " us to get the result " << sum << std::endl;
	}

	//virtual method on concrete instance
	{
		clock_t::time_point t0 = clock_t::now();
		int32_t sum = 0;
		for (int32_t i = 0; i < count; ++i)
			sum += val.val();
		clock_t::time_point t1 = clock_t::now();

		us_t time_span = chrono::duration_cast<us_t>(t1 - t0);
		std::cout << "\tvirtual method on instance: it takes " << time_span.count() << " us to get the result " << sum << std::endl;
	}

	//access to volatile
	{
		clock_t::time_point t0 = clock_t::now();
		int32_t sum = 0;
		for (int32_t i = 0; i < count; ++i)
			sum += ival;
		clock_t::time_point t1 = clock_t::now();

		us_t time_span = chrono::duration_cast<us_t>(t1 - t0);
		std::cout << "\taccess to volatile: it takes " << time_span.count() << " us to get the result " << sum << std::endl;
	}

	//access to atomic
	{
		clock_t::time_point t0 = clock_t::now();
		int32_t sum = 0;
		for (int32_t i = 0; i < count; ++i)
			sum += a_ival;
		clock_t::time_point t1 = clock_t::now();

		us_t time_span = chrono::duration_cast<us_t>(t1 - t0);
		std::cout << "\taccess to atoimic: it takes " << time_span.count() << " us to get the result " << sum << std::endl;
	}
}