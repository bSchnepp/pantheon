#include <gtest/gtest.h>

#include <kern.h>
#include <kern_runtime.hpp>
#include <kern_container.hpp>
#include <kern_integers.hpp>

#include <Devices/kern_drivers.hpp>

#ifndef DRIVER_TESTS_HPP_
#define DRIVER_TESTS_HPP_

TEST(InitDriver, Psci)
{
	/* Can we actually do this...? */
	char Area[1024];
	ClearBuffer(Area, 1024);
	InitDriver("psci", (UINT64)&Area);
	ASSERT_EQ(Area[0], 0);
}

TEST(FiniDriver, Pcie)
{
	char Area[1024];
	ClearBuffer(Area, 1024);
	Area[0] = 'e';
	Area[1] = 'c';
	Area[2] = 'a';
	Area[3] = 'm';
	FiniDriver("pcie", (UINT64)&Area);
	ASSERT_EQ(Area[0], 'e');
	ASSERT_EQ(Area[1], 'c');
	ASSERT_EQ(Area[2], 'a');
	ASSERT_EQ(Area[3], 'm');
}

#endif