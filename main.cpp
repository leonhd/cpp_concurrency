#include <stdio.h>
#include <stdint.h>
#include "test_atomics.h"

int32_t main(int32_t argc, const char** argv)
{
	atomics_tester_t::test1(5000);

	return 0;
}