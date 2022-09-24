#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#ifndef _KERN_ELF_RELOCATIONS_HPP_
#define _KERN_ELF_RELOCATIONS_HPP_

typedef enum DynamicType
{
	DT_NULL = 0,
	DT_NEEDED = 1,
	DT_PLTRELSZ = 2,
	DT_PLTGOT = 3,
	DT_STRTAB = 5,
	DT_SYMTAB = 6,
	DT_RELA = 7,
	DT_RELASZ = 8,
	DT_RELAENT = 9,
	DT_STRSZ = 10,
	DT_REL = 17,
	DT_RELENT = 19,

	DT_RELACOUNT = 0x6FFFFFF9,
	DT_RELCOUNT = 0x6FFFFFFA
}DynTag;

class DynInfo
{
public:
	[[nodiscard]] constexpr FORCE_INLINE INT64 Tag() const
	{
		return this->TagVal;
	}

	[[nodiscard]] constexpr FORCE_INLINE UINT64 Value() const
	{
		return this->ValueVal;
	}

	[[nodiscard]] constexpr FORCE_INLINE UINT64 Address() const
	{
		return this->AddressVal;
	}

private:
	INT64 TagVal;
	union 
	{
		UINT64 ValueVal;
		UINT64 AddressVal;
	};
	
};

class RelInfo
{
public:
	[[nodiscard]] constexpr FORCE_INLINE UINT64 Type() const
	{
		return (this->InfoVal >> 0) & 0xFFFFFFFF;
	}

	[[nodiscard]] constexpr FORCE_INLINE UINT64 Sym() const
	{
		return (this->InfoVal >> 32) & 0xFFFFFFFF;
	}

	[[nodiscard]] constexpr FORCE_INLINE UINT64 Address() const
	{
		return this->AddressVal;
	}

private:
	UINT64 AddressVal;
	UINT64 InfoVal;
};


class RelaInfo
{
public:
	[[nodiscard]] constexpr FORCE_INLINE INT64 Addend() const
	{
		return this->AddedVal;
	}

	[[nodiscard]] constexpr FORCE_INLINE UINT64 Type() const
	{
		return (this->InfoVal >> 0) & 0xFFFFFFFF;
	}

	[[nodiscard]] constexpr FORCE_INLINE UINT64 Sym() const
	{
		return (this->InfoVal >> 32) & 0xFFFFFFFF;
	}

	[[nodiscard]] constexpr FORCE_INLINE UINT64 Address() const
	{
		return this->AddressVal;
	}

private:
	UINT64 AddressVal;
	UINT64 InfoVal;
	INT64 AddedVal;
};

extern "C" void ApplyRelocations(UINT64 Base, const DynInfo *DynamicInfo);


#endif