#include "kern_cpu.hpp"
#include <kern_datatypes.hpp>

/**
 * \~english @brief Initializes a CoreInfo structure.
 * \~english @details Prepares a CoreInfo struct by initializing its
 * basic variables to an idle state, signalling that it is ready to have
 * a scheduler assigned to it to begin processing threads.
 * 
 * \~english @author Brian Schnepp
 */
void pantheon::CPU::InitCoreInfo(CoreInfo *Block)
{
	Block->CurState = CPU_STATE_IDLE;
	
	Block->CurThread = nullptr;
	Block->CurProcess = nullptr;
	Block->CurSched = nullptr;
}

/**
 * \~english @brief Gets the processor number of the current core
 * \~english @author Brian Schnepp
 */
UINT8 pantheon::CPU::GetProcessorNumber()
{
	/* TODO: Portability to riscv!!! */
	UINT64 RetVal;
	asm volatile ("mrs %0, mpidr_el1\n" : "=r"(RetVal) ::);
	return (UINT8)(RetVal & 0xFF);
}