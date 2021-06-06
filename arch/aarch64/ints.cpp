#include "ints.hpp"
#include "gic.hpp"

#include <kern.h>
#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>
#include <Common/Proc/kern_cpu.hpp>

static UINT64 TimerClock = 1000;

extern "C" void sync_handler_el1_sp0()
{
	SERIAL_LOG("%s\n", "ERR: SYNC HANDLER EL1 SP0");
}

extern "C" void err_handler_el1_sp0()
{
	SERIAL_LOG("%s\n", "ERR: ERR HANDLER EL1 SP0");
}

extern "C" void fiq_handler_el1_sp0()
{
	SERIAL_LOG("%s\n", "ERR: FIQ HANDLER EL1 SP0");
}

extern "C" void irq_handler_el1_sp0()
{
	SERIAL_LOG("%s\n", "ERR: IRQ HANDLER EL1 SP0");
}


extern "C" void sync_handler_el1()
{
	SERIAL_LOG("%s\n", "ERR: SYNC HANDLER EL1");
}

extern "C" void err_handler_el1()
{
	SERIAL_LOG("%s\n", "ERR: ERR HANDLER EL1");
}

extern "C" void fiq_handler_el1()
{
	SERIAL_LOG("%s\n", "ERR: FIQ HANDLER EL1");
}

extern "C" void irq_handler_el1()
{
	UINT32 IAR = pantheon::arm::GICRecvInterrupt();
	pantheon::arm::GICAckInterrupt(IAR);
	if ((IAR & 0x3FF) == 30)
	{
		pantheon::arm::RearmSystemTimer(TimerClock);
	}
}


extern "C" void sync_handler_el0()
{
	SERIAL_LOG("%s\n", "ERR: SYNC HANDLER EL0");
}

extern "C" void err_handler_el0()
{
	SERIAL_LOG("%s\n", "ERR: ERR HANDLER EL0");
}

extern "C" void fiq_handler_el0()
{
	SERIAL_LOG("%s\n", "ERR: FIQ HANDLER EL0");
}

extern "C" void irq_handler_el0()
{
	SERIAL_LOG("%s\n", "ERR: IRQ HANDLER EL0");
}


extern "C" void sync_handler_el0_32()
{
	SERIAL_LOG("%s\n", "ERR: SYNC HANDLER EL0_32");
}

extern "C" void err_handler_el0_32()
{
	SERIAL_LOG("%s\n", "ERR: ERR HANDLER EL0_32");
}

extern "C" void fiq_handler_el0_32()
{
	SERIAL_LOG("%s\n", "ERR: FIQ HANDLER EL0_32");
}

extern "C" void irq_handler_el0_32()
{
	SERIAL_LOG("%s\n", "ERR: IRQ HANDLER EL0_32");
}

VOID pantheon::arm::LoadInterruptTable(VOID *Table)
{
	asm volatile ("msr vbar_el1, %0\n" :: "r"(Table) : "memory");
	asm volatile("msr daifclr, #15\n");
}

UINT64 pantheon::arm::GetSystemTimerClock()
{
	volatile UINT64 Ret;
	asm volatile ("mrs %0, cntfrq_el0\n" : "=r"(Ret));
	return Ret;
}

void pantheon::arm::RearmSystemTimer()
{
	UINT64 ClockSpeed;
	asm volatile ("mrs %0, cntfrq_el0\n" : "=r"(ClockSpeed));
	ClockSpeed /= TimerClock;
	
	volatile UINT64 TimerCtl = 1;

	asm volatile ("msr cntp_tval_el0, %0\n" 
			"msr cntp_ctl_el0, %1"
			:: "r"(ClockSpeed), "r"(TimerCtl) : "memory");
}

VOID pantheon::arm::RearmSystemTimer(UINT64 Frequency)
{
	TimerClock = Frequency;
	pantheon::arm::RearmSystemTimer();
}

VOID pantheon::arm::DisableSystemTimer()
{
	UINT64 ClockSpeed = 0xFFFFFFFFFFFFFFFF;
	volatile UINT64 TimerCtl = 1;
	asm volatile ("msr cntp_tval_el0, %0\n" 
			"msr cntp_ctl_el0, %1"
			:: "r"(ClockSpeed), "r"(TimerCtl) : "memory");	
}

VOID pantheon::arm::CLI()
{
	pantheon::arm::GICDisable();
}

VOID pantheon::arm::STI()
{
	pantheon::arm::GICEnable();
}