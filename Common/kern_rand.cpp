#include <kern_datatypes.hpp>
#include <Common/kern_rand.hpp>
#include <Common/Sync/kern_atomic.hpp>


/**
 * @brief An implementation of Wichmann-Hill random number generation
 * @return A (cryptographically insecure) random number
 */
UINT64 pantheon::Rand()
{
	/* Arbitrary seeds. These were randomly generated. */
	static Atomic<UINT64> Seeds[3] = 
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
		0xD398EB8584DBF6d2,
		0x2C1AF76BB6FADB0E,
	};

	for (UINT8 Index = 0; Index < 3; Index++)
	{
		Seeds[0].Store(Mults[Index] * Seeds[Index].Load() + Additions[Index]);
	}

	return Seeds[0].Load() ^ Seeds[1].Load() ^ Seeds[2].Load();
}