#include "thread.hpp"
#include <kern_datatypes.hpp>

#ifndef _AARCH64_INTS_HPP_
#define _AARCH64_INTS_HPP_

namespace pantheon::arm
{

VOID LoadInterruptTable(VOID *Table);

UINT64 GetSystemTimerClock();

VOID RearmSystemTimer();
VOID RearmSystemTimer(UINT64 Frequency);
VOID DisableSystemTimer();

VOID CLI();
VOID STI();

UINT64 DAIFR();

}

extern "C" void sync_handler_el1_sp0(pantheon::arm::TrapFrame *Frame);
extern "C" void err_handler_el1_sp0(pantheon::arm::TrapFrame *Frame);
extern "C" void fiq_handler_el1_sp0(pantheon::arm::TrapFrame *Frame);
extern "C" void irq_handler_handler_el1_sp0(pantheon::arm::TrapFrame *Frame);
extern "C" void sync_handler_el1(pantheon::arm::TrapFrame *Frame);
extern "C" void err_handler_el1(pantheon::arm::TrapFrame *Frame);
extern "C" void fiq_handler_el1(pantheon::arm::TrapFrame *Frame);
extern "C" void irq_handler_handler_el1(pantheon::arm::TrapFrame *Frame);
extern "C" void sync_handler_el0(pantheon::arm::TrapFrame *Frame);
extern "C" void err_handler_el0(pantheon::arm::TrapFrame *Frame);
extern "C" void fiq_handler_el0(pantheon::arm::TrapFrame *Frame);
extern "C" void irq_handler_handler_el0(pantheon::arm::TrapFrame *Frame);
extern "C" void sync_handler_el0_32(pantheon::arm::TrapFrame *Frame);
extern "C" void err_handler_el0_32(pantheon::arm::TrapFrame *Frame);
extern "C" void fiq_handler_el0_32(pantheon::arm::TrapFrame *Frame);
extern "C" void irq_handler_handler_el0_32(pantheon::arm::TrapFrame *Frame);

#endif