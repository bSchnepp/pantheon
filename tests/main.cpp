#include <iostream>

#include "gtest/gtest.h"

#include "common_tests.hpp"
#include "struct_tests.hpp"
#include "sched_tests.hpp"
#include "arch_tests.hpp"

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}