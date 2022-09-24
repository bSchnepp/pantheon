#include <stdint.h>

#ifndef _BYTE_SWAP_HPP_
#define _BYTE_SWAP_HPP_

inline uint8_t SwapBytes(uint8_t Item)
{
	return Item;
}

inline uint16_t SwapBytes(uint16_t Item)
{
	return __builtin_bswap16(Item);
}

inline uint32_t SwapBytes(uint32_t Item)
{
	return __builtin_bswap32(Item);
}

inline uint64_t SwapBytes(uint64_t Item)
{
	return __builtin_bswap64(Item);
}

inline int8_t SwapBytes(int8_t Item)
{
	return Item;
}

inline int16_t SwapBytes(int16_t Item)
{
	uint16_t UnsignedItem = Item;
	uint16_t Swap = __builtin_bswap16(UnsignedItem);
	return (int16_t)(Swap);
}

inline int32_t SwapBytes(int32_t Item)
{
	uint32_t UnsignedItem = Item;
	uint32_t Swap = __builtin_bswap32(UnsignedItem);
	return (int32_t)(Swap);
}

inline int64_t SwapBytes(int64_t Item)
{
	uint64_t UnsignedItem = Item;
	uint64_t Swap = __builtin_bswap64(UnsignedItem);
	return (int64_t)(Swap);
}



#endif