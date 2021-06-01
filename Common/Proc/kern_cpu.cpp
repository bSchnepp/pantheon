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