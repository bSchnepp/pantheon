#include "ints.hpp"
#include "gic.hpp"

#include <kern.h>
#include <vmm/vmm.hpp>

#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#include <Proc/kern_cpu.hpp>
#include <Proc/kern_sched.hpp>
#include <Proc/kern_thread.hpp>
#include <System/Syscalls/Syscalls.hpp>

static UINT64 TimerClock = 1000;

VOID TraceStack(UINT64 X29)
{
	struct Frame
	{
		Frame *Next;
		UINT64 PC;
	};

	UINT64 Counter = 0;
	Frame *Cur = reinterpret_cast<Frame*>(X29);
	while (Cur && Cur->PC)
	{
		SERIAL_LOG("[Frame %lu]: %lx\n", Counter++, Cur->PC);
		Cur = Cur->Next;
	}
}

extern "C" void sync_handler_el1_sp0(pantheon::TrapFrame *Frame)
{
	PANTHEON_UNUSED(Frame);
	pantheon::StopError("ERR: SYNC HANDLER EL1 SP0");
}

extern "C" void err_handler_el1_sp0(pantheon::TrapFrame *Frame)
{
	PANTHEON_UNUSED(Frame);
	pantheon::StopError("ERR: ERR HANDLER EL1 SP0");
}

extern "C" void fiq_handler_el1_sp0(pantheon::TrapFrame *Frame)
{
	PANTHEON_UNUSED(Frame);
	SERIAL_LOG("%s\n", "ERR: FIQ HANDLER EL1 SP0");
}

extern "C" void irq_handler_el1_sp0(pantheon::TrapFrame *Frame)
{
	PANTHEON_UNUSED(Frame);
	SERIAL_LOG("%s\n", "ERR: IRQ HANDLER EL1 SP0");
}

extern "C" void sync_handler_el1(pantheon::TrapFrame *Frame)
{
	UINT64 ESR, FAR, ELR, SPSR, SP;
	asm volatile(
		"mrs %0, esr_el1\n"
		"mrs %1, far_el1\n"
		"mrs %2, elr_el1\n"
		"mrs %3, spsr_el1\n"
		"mov %4, sp\n"
		: "=r"(ESR), "=r"(FAR), "=r"(ELR), "=r"(SPSR), "=r"(SP));

	pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();
	pantheon::Process *CurProc = pantheon::CPU::GetCurProcess();

	TraceStack(Frame->Regs[29]);
	for (UINT64 Index = 0; Index < 31; Index++)
	{
		SERIAL_LOG("x%lu: %lx ", Index, Frame->Regs[Index]);
	}
	SERIAL_LOG("\n");

	pantheon::StopErrorFmt(
		"ERR: SYNC HANDLER EL1: esr: %lx far: %lx elr: %lx spsr: %lx, sp: %lx pid: %u, tid: %lu\n", 
		ESR, FAR, ELR, SPSR, SP, CurProc->ProcessID(), CurThread->ThreadID());
}

extern "C" void err_handler_el1(pantheon::TrapFrame *Frame)
{
	TraceStack(Frame->Regs[29]);
	pantheon::StopError("ERR: ERR HANDLER EL1");
}

extern "C" void fiq_handler_el1(pantheon::TrapFrame *Frame)
{
	PANTHEON_UNUSED(Frame);
	SERIAL_LOG("%s\n", "ERR: FIQ HANDLER EL1");
}

class CurFrameMgr
{
public:
	CurFrameMgr(pantheon::TrapFrame *Frame) { pantheon::CPU::GetCoreInfo()->CurFrame = Frame; }
	~CurFrameMgr() { pantheon::CPU::GetCoreInfo()->CurFrame = nullptr; }
};


extern "C" void irq_handler_el1(pantheon::TrapFrame *Frame)
{
	CurFrameMgr _M(Frame);

	UINT32 IAR = pantheon::arm::GICRecvInterrupt();
	pantheon::arm::GICAckInterrupt(IAR);
	if ((IAR & 0x3FF) == 30)
	{
		pantheon::CPU::CoreInfo *CoreInfo = pantheon::CPU::GetCoreInfo();
		CoreInfo->LocalJiffies++;

		pantheon::arm::RearmSystemTimer();
		pantheon::Scheduler::AttemptReschedule();
	}
}

extern "C" void enable_interrupts();
extern "C" void disable_interrupts();

extern "C" void sync_handler_el0(pantheon::TrapFrame *Frame)
{
	CurFrameMgr _M(Frame);
	UINT64 ESR, FAR, ELR, SPSR;
	asm volatile(
		"mrs %0, esr_el1\n"
		"mrs %1, far_el1\n"
		"mrs %2, elr_el1\n"
		"mrs %3, spsr_el1\n"
		: "=r"(ESR), "=r"(FAR), "=r"(ELR), "=r"(SPSR));

	UINT64 ESRType = (ESR >> 26);
	if ((ESRType & 0xFF) == 0x15)
	{
		pantheon::CPU::STI();
		UINT32 SyscallNo = Frame->Regs[8];
		pantheon::CallSyscall(SyscallNo, Frame);
		pantheon::CPU::CLI();
	} 
	else
	{
		pantheon::Thread *CurThread = pantheon::CPU::GetCurThread();
		SERIAL_LOG("PID %d: Bad sync handler el0: esr: 0x%lx far: 0x%lx elr: 0x%lx spsr: 0x%lx\n", CurThread->MyProc()->ProcessID(), ESR, FAR, ELR, SPSR);
		if (ESR == 0x2000000)
		{
			pantheon::vmm::PageTable *PT = CurThread->MyProc()->GetPageTable();
			pantheon::vmm::PrintPageTablesNoZeroes(PT);
		}
	} 
}

extern "C" void err_handler_el0(pantheon::TrapFrame *Frame)
{
	PANTHEON_UNUSED(Frame);
	SERIAL_LOG("%s\n", "ERR: ERR HANDLER EL0");
}

extern "C" void fiq_handler_el0(pantheon::TrapFrame *Frame)
{
	PANTHEON_UNUSED(Frame);
	SERIAL_LOG("%s\n", "ERR: FIQ HANDLER EL0");
}

extern "C" void irq_handler_el0(pantheon::TrapFrame *Frame)
{
	irq_handler_el1(Frame);
}


extern "C" void sync_handler_el0_32(pantheon::TrapFrame *Frame)
{
	PANTHEON_UNUSED(Frame);
	SERIAL_LOG("%s\n", "ERR: SYNC HANDLER EL0_32");
}

extern "C" void err_handler_el0_32(pantheon::TrapFrame *Frame)
{
	PANTHEON_UNUSED(Frame);
	SERIAL_LOG("%s\n", "ERR: ERR HANDLER EL0_32");
}

extern "C" void fiq_handler_el0_32(pantheon::TrapFrame *Frame)
{
	PANTHEON_UNUSED(Frame);
	SERIAL_LOG("%s\n", "ERR: FIQ HANDLER EL0_32");
}

extern "C" void irq_handler_el0_32(pantheon::TrapFrame *Frame)
{
	PANTHEON_UNUSED(Frame);
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

UINT64 pantheon::arm::DAIFR()
{
	UINT64 Val;
	asm volatile("mrs %0, daif\n" : "=r"(Val));
	return Val;
}