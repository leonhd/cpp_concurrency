#include <stdio.h>
#include <stdint.h>
#include "test_atomics.h"
#include "test_sync.h"
#include "test_normal.h"

int32_t main(int32_t argc, const char** argv)
{
	atomics_tester_t::test_atomics_rb<int64_t>(1000000);
	return 0;

	sync_tester_t::test_sync_queue<int32_t>(1000000);
	return 0;

	normal_tester_t::test_mem_access(10000000);
	return 0;

	atomics_tester_t::test2(1000);
	return 0;
}
