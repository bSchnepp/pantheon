#include <kern_datatypes.hpp>

#include <System/Proc/kern_cpu.hpp>

#include <Common/kern_rand.hpp>
#include <Common/Sync/kern_atomic.hpp>
#include <Common/Sync/kern_lockable.hpp>

/* Do something about this at some point... */
static pantheon::Spinlock RNGLock("Random Number Lock");

/**
 * @brief An implementation of Wichmann-Hill random number generation
 * @return A (cryptographically insecure) random number
 */
UINT64 pantheon::Rand()
{
	RNGLock.Acquire();

	/* Arbitrary seeds. These were randomly generated. */
	static UINT64 Seeds[3] = 
	{ 
		0x5648AD4DA64862EB, 
		0xF7293DDAD5921465, 
		0xC2743D545EDD6E10 
	};

	static const UINT64 Mults[3] = 
	{
		0x043186A3,
		0xA74CFF4D,
		0xDE834306,
	};

	static const UINT64 Additions[3] =
	{
		0xC14A13C7025F69E3,
		0xD398EB8584DBF6D2,
		0x2C1AF76BB6FADB0E,
	};

	/* Use jiffies to help make this more random */
	UINT64 Jiffies = pantheon::CPU::GetJiffies() % 16;
	for (UINT64 Counter = 0; Counter < Jiffies; Counter++)
	{
		for (UINT8 Index = 0; Index < 3; Index++)
		{
			Seeds[Index] = Jiffies + (Mults[Index] * Seeds[Index] + Additions[Index]);
		}
	}

	UINT64 Result = Seeds[0] ^ Seeds[1] ^ Seeds[2];
	RNGLock.Release();
	return Result;
}