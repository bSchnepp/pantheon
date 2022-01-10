#include "kern_cpu.hpp"

#include <vmm/vmm.hpp>
#include <Boot/Boot.hpp>
#include <kern_datatypes.hpp>
#include <PhyMemory/kern_alloc.hpp>

static pantheon::GlobalScheduler GlobalSched;

/* Pantheon can have up to 256 processors in theory.
 * In practice, this should probably be cut down to 8 or 16, which is
 * way more realistic for a SoM I can actually buy. 
 * 256 thread x86 systems barely exist, so it's highly unlikely for any aarch64
 * systems with that many cores or more to exist.
 */
static pantheon::CPU::CoreInfo PerCoreInfo[256];

pantheon::CPU::CoreInfo *pantheon::CPU::GetCoreInfo()
{
	return &(PerCoreInfo[pantheon::CPU::GetProcessorNumber()]);
}

/**
 * \~english @brief Initializes a CoreInfo structure.
 * \~english @details Prepares a CoreInfo struct by initializing its
 * basic variables to an idle state, signalling that it is ready to have
 * a scheduler assigned to it to begin processing threads.
 * 
 * \~english @author Brian Schnepp
 */
void pantheon::CPU::InitCoreInfo(UINT8 CoreNo)
{
	PerCoreInfo[CoreNo].CurFrame = nullptr;
	PerCoreInfo[CoreNo].NOff = 0;

	void *MaybeAddr = BasicMalloc(sizeof(pantheon::Scheduler))();
	if (!MaybeAddr)
	{
		SERIAL_LOG("%s\n", "unable to malloc scheduler!!!!");
		return;
	}

	PerCoreInfo[CoreNo].CurSched = reinterpret_cast<Scheduler*>(MaybeAddr);
	(*PerCoreInfo[CoreNo].CurSched) = pantheon::Scheduler();
}

pantheon::GlobalScheduler *pantheon::CPU::GetGlobalScheduler()
{
	return &(GlobalSched);
}

pantheon::Thread *pantheon::CPU::GetCurThread()
{
	return pantheon::CPU::GetCoreInfo()->CurSched->MyThread();
}

pantheon::Scheduler *pantheon::CPU::GetCurSched()
{
	return pantheon::CPU::GetCoreInfo()->CurSched;
}

pantheon::TrapFrame *pantheon::CPU::GetCurFrame()
{
	return pantheon::CPU::GetCoreInfo()->CurFrame;
}


extern "C" VOID drop_usermode(UINT64 PC, UINT64 PSTATE, UINT64 SP);
BOOL pantheon::CPU::DropToUsermode(UINT64 PC)
{
	pantheon::CPU::GetCurThread()->Lock();
	pantheon::vmm::PageTableEntry UEntry;
	UEntry.SetPagePermissions(0b01 << 6);
	UEntry.SetKernelNoExecute(TRUE);
	UEntry.SetUserNoExecute(TRUE);
	UEntry.SetSharable(pantheon::vmm::PAGE_SHARABLE_TYPE_INNER);
	UEntry.SetAccessor(pantheon::vmm::PAGE_MISC_ACCESSED);
	UEntry.SetMAIREntry(pantheon::vmm::MAIREntry_1);

	UINT64 StackArea = pantheon::PageAllocator::Alloc();
	pantheon::CPU::GetCurThread()->SetUserStackAddr(StackArea);

	/* TODO: actually get this process' TTBR0 */
	alignas(0x1000) static pantheon::vmm::PageTable UserStacks[128];
	static pantheon::vmm::PageAllocator BaseAllocator(UserStacks, 128);

	pantheon::vmm::PageTable *PT = (pantheon::vmm::PageTable *)pantheon::CPUReg::R_TTBR0_EL1();
	BaseAllocator.Reprotect(PT, StackArea, pantheon::vmm::BlockSize::L3BlockSize, UEntry);

	StackArea += pantheon::vmm::BlockSize::L3BlockSize;
	pantheon::CPU::GetCurThread()->Unlock();
	drop_usermode(PC, 0, StackArea);
	return TRUE;
}

alignas(4096) static char StackArea[MAX_NUM_CPUS * DEFAULT_STACK_SIZE];

void *pantheon::CPU::GetStackArea(UINT64 Core)
{
	return StackArea + static_cast<UINT64>(Core * DEFAULT_STACK_SIZE);
}


extern "C" void *get_stack_area()
{
	UINT8 CpuNo = pantheon::CPU::GetProcessorNumber();
	return pantheon::CPU::GetStackArea(CpuNo);
}