#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#include <System/Exec/kern_elf.hpp>
#include <System/Exec/kern_elf_relocations.hpp>

extern "C" void ApplyRelocations(UINT64 Base, const DynInfo *DynamicInfo)
{
	enum DynLocation
	{
		REL = 0,
		RELA,
		RELCOUNT,
		RELACOUNT,
		RELENT,
		RELAENT,
		REL_MAX
	};

	UINT64 Items[REL_MAX] = {0};

	for (const auto *Current = DynamicInfo; Current->Tag() != DT_NULL; Current++)
	{
		UINT64 Tag = Current->Tag();
		switch (Tag)
		{
		case DT_REL:
			Items[REL] = Base + Current->Address();
			break;

		case DT_RELA:
			Items[RELA] = Base + Current->Address();
			break;

		case DT_RELENT:
			Items[RELENT] = Current->Value();
			break;

		case DT_RELAENT:
			Items[RELAENT] = Current->Value();
			break;

		case DT_RELCOUNT:
			Items[RELCOUNT] = Current->Value();
			break;

		case DT_RELACOUNT:
			Items[RELACOUNT] = Current->Value();
			break;

		default:
			break;
		}
	}

	for (UINT64 Index = 0; Index < Items[RELCOUNT]; Index++)
	{
		UINT64 EntryAddr = Items[REL] + (Items[RELENT] * Index);
		const RelInfo *Entry = reinterpret_cast<const RelInfo *>(EntryAddr);
		if (Entry && Entry->Type() == pantheon::exec::R_AARCH64_RELATIVE)
		{
			UINT64 *WriteAddress = reinterpret_cast<UINT64*>(Base + Entry->Address());
			*WriteAddress += Base;
		} 
		else 
		{
			pantheon::StopError("Bad RELENT");
		}
	}

	for (UINT64 Index = 0; Index < Items[RELACOUNT]; Index++)
	{
		UINT64 EntryAddr = Items[RELA] + (Items[RELAENT] * Index);
		const RelaInfo *Entry = reinterpret_cast<const RelaInfo *>(EntryAddr);
		if (Entry && Entry->Type() == pantheon::exec::R_AARCH64_RELATIVE)
		{
			UINT64 *WriteAddress = reinterpret_cast<UINT64*>(Base + Entry->Address());
			*WriteAddress = Base + Entry->Addend();
		}
		else 
		{
			pantheon::StopError("Bad RELAENT");
		}
	}
}