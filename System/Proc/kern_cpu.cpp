#include "kern_cpu.hpp"
#include <kern_datatypes.hpp>

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
	PerCoreInfo[CoreNo].CurThread = nullptr;
	PerCoreInfo[CoreNo].CurFrame = nullptr;

	void *MaybeAddr = BasicMalloc(sizeof(pantheon::Scheduler))();
	if (!MaybeAddr)
	{
		SERIAL_LOG("%s\n", "unable to malloc scheduler!!!!");
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
	return pantheon::CPU::GetCoreInfo()->CurThread;
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
	UINT64 StackLoc = 12 * 1024;
	Optional<void*> StackArea = BasicMalloc(StackLoc);
	if (!StackArea.GetOkay())
	{
		return FALSE;
	}
	void *StackAreaAddr = StackArea();
	StackLoc += ((UINT64)StackAreaAddr);
	drop_usermode(PC, 0, StackLoc);
	return TRUE;
}