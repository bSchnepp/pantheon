#include <gtest/gtest.h>

#include <kern.h>
#include <arch.hpp>
#include <kern_runtime.hpp>
#include <kern_container.hpp>
#include <kern_integers.hpp>

#ifndef ARCH_TESTS_HPP_
#define ARCH_TESTS_HPP_


TEST(MockCpuNo, CpuNo)
{
	/* Stub for tests. */
	ASSERT_EQ(pantheon::CPU::GetProcessorNumber(), 0);
}


#endif