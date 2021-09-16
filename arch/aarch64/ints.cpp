#include "ints.hpp"
#include "gic.hpp"

#include <kern.h>
#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>
#include <Proc/kern_cpu.hpp>

static UINT64 TimerClock = 1000;

extern "C" void sync_handler_el1_sp0(pantheon::TrapFrame *Frame)
{
	pantheon::CPU::GetCoreInfo()->CurFrame = Frame;
	SERIAL_LOG_UNSAFE("%s\n", "ERR: SYNC HANDLER EL1 SP0");
	pantheon::CPU::GetCoreInfo()->CurFrame = nullptr;
}

extern "C" void err_handler_el1_sp0(pantheon::TrapFrame *Frame)
{
	pantheon::CPU::GetCoreInfo()->CurFrame = Frame;
	SERIAL_LOG_UNSAFE("%s\n", "ERR: ERR HANDLER EL1 SP0");
	pantheon::CPU::GetCoreInfo()->CurFrame = nullptr;
}

extern "C" void fiq_handler_el1_sp0(pantheon::TrapFrame *Frame)
{
	pantheon::CPU::GetCoreInfo()->CurFrame = Frame;
	SERIAL_LOG_UNSAFE("%s\n", "ERR: FIQ HANDLER EL1 SP0");
	pantheon::CPU::GetCoreInfo()->CurFrame = nullptr;
}

extern "C" void irq_handler_el1_sp0(pantheon::TrapFrame *Frame)
{
	pantheon::CPU::GetCoreInfo()->CurFrame = Frame;
	SERIAL_LOG_UNSAFE("%s\n", "ERR: IRQ HANDLER EL1 SP0");
	pantheon::CPU::GetCoreInfo()->CurFrame = nullptr;
}

extern "C" void sync_handler_el1(pantheon::TrapFrame *Frame)
{
	pantheon::CPU::GetCoreInfo()->CurFrame = Frame;
	SERIAL_LOG_UNSAFE("%s\n", "ERR: SYNC HANDLER EL1");
	pantheon::CPU::GetCoreInfo()->CurFrame = nullptr;
}

extern "C" void err_handler_el1(pantheon::TrapFrame *Frame)
{
	pantheon::CPU::GetCoreInfo()->CurFrame = Frame;
	SERIAL_LOG_UNSAFE("%s\n", "ERR: ERR HANDLER EL1");
	pantheon::CPU::GetCoreInfo()->CurFrame = nullptr;
}

extern "C" void fiq_handler_el1(pantheon::TrapFrame *Frame)
{
	pantheon::CPU::GetCoreInfo()->CurFrame = Frame;
	SERIAL_LOG_UNSAFE("%s\n", "ERR: FIQ HANDLER EL1");
	pantheon::CPU::GetCoreInfo()->CurFrame = nullptr;
}

extern "C" void irq_handler_el1(pantheon::TrapFrame *Frame)
{
	pantheon::CPU::GetCoreInfo()->CurFrame = Frame;
	UINT32 IAR = pantheon::arm::GICRecvInterrupt();
	pantheon::arm::GICAckInterrupt(IAR);
	if ((IAR & 0x3FF) == 30)
	{
		pantheon::CPU::CoreInfo *CoreData = pantheon::CPU::GetCoreInfo();
		pantheon::Scheduler *CurSched = CoreData->CurSched;
		pantheon::Thread *CurThread = CurSched->MyThread();
		pantheon::arm::RearmSystemTimer();

		UINT64 RemainingTicks = 0;
		if (CurThread)
		{
			CurThread->CountTick();
			RemainingTicks = CurThread->TicksLeft();
		}
		
		if (RemainingTicks == 0)
		{
			CurSched->SignalReschedule();
		}

		CurSched->MaybeReschedule();
	}
	pantheon::CPU::GetCoreInfo()->CurFrame = nullptr;
}


/* sync_handler_el0 is entirely in asm */

extern "C" void err_handler_el0(pantheon::TrapFrame *Frame)
{
	pantheon::CPU::GetCoreInfo()->CurFrame = Frame;
	SERIAL_LOG_UNSAFE("%s\n", "ERR: ERR HANDLER EL0");
	pantheon::CPU::GetCoreInfo()->CurFrame = nullptr;
}

extern "C" void fiq_handler_el0(pantheon::TrapFrame *Frame)
{
	pantheon::CPU::GetCoreInfo()->CurFrame = Frame;
	SERIAL_LOG_UNSAFE("%s\n", "ERR: FIQ HANDLER EL0");
	pantheon::CPU::GetCoreInfo()->CurFrame = nullptr;
}

extern "C" void irq_handler_el0(pantheon::TrapFrame *Frame)
{
	irq_handler_el1(Frame);
}


extern "C" void sync_handler_el0_32(pantheon::TrapFrame *Frame)
{
	pantheon::CPU::GetCoreInfo()->CurFrame = Frame;
	SERIAL_LOG_UNSAFE("%s\n", "ERR: SYNC HANDLER EL0_32");
	pantheon::CPU::GetCoreInfo()->CurFrame = nullptr;
}

extern "C" void err_handler_el0_32(pantheon::TrapFrame *Frame)
{
	pantheon::CPU::GetCoreInfo()->CurFrame = Frame;
	SERIAL_LOG_UNSAFE("%s\n", "ERR: ERR HANDLER EL0_32");
	pantheon::CPU::GetCoreInfo()->CurFrame = nullptr;
}

extern "C" void fiq_handler_el0_32(pantheon::TrapFrame *Frame)
{
	pantheon::CPU::GetCoreInfo()->CurFrame = Frame;
	SERIAL_LOG_UNSAFE("%s\n", "ERR: FIQ HANDLER EL0_32");
	pantheon::CPU::GetCoreInfo()->CurFrame = nullptr;
}

extern "C" void irq_handler_el0_32(pantheon::TrapFrame *Frame)
{
	pantheon::CPU::GetCoreInfo()->CurFrame = Frame;
	SERIAL_LOG_UNSAFE("%s\n", "ERR: IRQ HANDLER EL0_32");
	pantheon::CPU::GetCoreInfo()->CurFrame = nullptr;
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

UINT64 pantheon::arm::DAIFR()
{
	UINT64 Val;
	asm volatile("mrs %0, daif\n" : "=r"(Val));
	return Val;
}

VOID pantheon::arm::CLI()
{
	asm volatile("msr daifset, #3\n");
}

VOID pantheon::arm::STI()
{
	asm volatile("msr daifclr, #3\n");
}