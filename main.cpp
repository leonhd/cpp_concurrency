#include <stdio.h>
#include <stdint.h>
#include "test_atomics.h"
#include "test_sync.h"
#include <chrono>
using namespace std;

struct val_interface
{
	virtual int32_t val() = 0;
};

struct val_t : public val_interface{
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
};
val_t val(0);
volatile int32_t ival = 0;

int32_t main(int32_t argc, const char** argv)
{
	int64_t freq, t0, t1;
	::QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	::QueryPerformanceCounter((LARGE_INTEGER*)&t0);
	int32_t sum = 0;
	val_interface *val_ptr = new val_t(0);
	for (int32_t i = 0; i < 100000000; ++i)
		sum += ival;
	::QueryPerformanceCounter((LARGE_INTEGER*)&t1);
	
	std::cout << "it takes " << (t1 - t0) * 1000 / freq << " mili-seconds to get the result " << sum << std::endl;
	return 0;

	sync_tester_t::test_atomic_queue<int32_t>(1000);
	return 0;

	atomics_tester_t::test2(1000);
	return 0;
}