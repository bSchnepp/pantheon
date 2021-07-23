#include <iostream>

#include "gtest/gtest.h"

#include "common_tests.hpp"
#include "struct_tests.hpp"
#include "sched_tests.hpp"
#include "driver_tests.hpp"
#include "arch_tests.hpp"

/* Necessary to silence linker error */
extern "C" void _putchar(char c)
{
	std::cout << c << std::endl;
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}