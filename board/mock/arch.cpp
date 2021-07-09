#include <kern.h>

#include <arch.hpp>
#include <arch/aarch64/ints.hpp>
#include <arch/aarch64/thread.hpp>

#include <System/Syscalls/Syscalls.hpp>
#include <System/Memory/kern_physpaging.hpp>

void createprocess_tail()
{

}

void cpu_switch(pantheon::CpuContext *Old, pantheon::CpuContext *New, UINT32 RegOffset)
{
	
}

UINT8 pantheon::CPU::GetProcessorNumber()
{
	return 0;
}

VOID pantheon::CPU::CLI()
{
}

VOID pantheon::CPU::STI()
{
}

VOID pantheon::CPU::PAUSE()
{

}

VOID pantheon::RearmSystemTimer()
{

}

VOID pantheon::RearmSystemTimer(UINT64 Freq)
{

}

VOID pantheon::DisableSystemTimer()
{

}

extern "C" INT32 CallSMC(UINT64 X0, UINT64 X1, UINT64 X2, UINT64 X3)
{
	return 0;
}

extern "C" INT32 CallHVC(UINT64 X0, UINT64 X1, UINT64 X2, UINT64 X3)
{
	return 0;
}

extern "C" VOID asm_kern_init_core()
{

}

extern "C" VOID drop_usermode(UINT64 PC)
{
	PANTHEON_UNUSED(PC);
}

extern "C" UINT64 svc_LogText(const CHAR *Text)
{
	return pantheon::SVCLogText(Text);
}

VOID PerCoreBoardInit()
{
	
}


pantheon::vmm::PageTableEntry pantheon::vmm::CreateEntry(
	const PageTableEntry *NextLevel, 
	PageGranularity Size, 
	PageAccessor Accessor, 
	UINT64 Permission, 
	PageSharableType Sharable, 
	PageTypeMMIOAccessor MMIOType)
{
	pantheon::vmm::PageTableEntry Entry = 0;

	UINT64 FinalAddr = (UINT64)NextLevel;
	FinalAddr &= ~0xFFFULL;			/* Wipe lower bits */
	FinalAddr &= ~(0x1FFEULL << 52);	/* Wipe upper bits */
	Entry |= FinalAddr;			/* And write the address in */

	/* For each attribute given, also put that in. */
	Entry |= Size;
	Entry |= Accessor;
	Entry |= Permission;
	Entry |= Sharable;
	Entry |= MMIOType;

	return Entry;
}

pantheon::vmm::PageTable *pantheon::vmm::CreateBasicPageTables()
{
	Optional<UINT64> MaybeAddr = pantheon::GetGlobalPhyManager()->FindAndClaimFirstFreeAddress();
	if (!MaybeAddr.GetOkay())
	{
		return nullptr;
	}
	
	auto *Table = (pantheon::vmm::PageTable *)(MaybeAddr.GetValue());
	ClearBuffer((CHAR*)Table, pantheon::PhyPageManager::PageSize());
	return Table;
}