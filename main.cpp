#include <stdio.h>
#include <stdint.h>
#include "test_atomics.h"
#include "test_sync.h"

int32_t main(int32_t argc, const char** argv)
{
	sync_tester_t::test_atomic_queue<int32_t>(1000000);
	return 0;
	atomics_tester_t::test2(1000);
	return 0;
}