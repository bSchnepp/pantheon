#include <gtest/gtest.h>

#include <kern_runtime.hpp>
#include <kern_container.hpp>

#include <Common/Proc/kern_cpu.hpp>
#include <Common/Proc/kern_proc.hpp>
#include <Common/Proc/kern_sched.hpp>
#include <Common/Proc/kern_thread.hpp>

#ifndef SCHED_TESTS_HPP_
#define SCHED_TESTS_HPP_

TEST(Scheduler, CreateThread)
{
	pantheon::Thread T(nullptr);
	ASSERT_EQ(T.MyProc(), nullptr);
}

TEST(Scheduler, CreateThreadWithProc)
{
	pantheon::Process Proc;
	pantheon::Thread T(&Proc);
	ASSERT_EQ(T.MyProc(), &Proc);
}

TEST(Scheduler, CreateThreadWithProcCopy)
{
	pantheon::Process Proc("some name");
	pantheon::Thread T(&Proc);
	ASSERT_EQ(T.MyProc(), &Proc);

	pantheon::Process Proc2 = Proc;
	ASSERT_EQ(Proc2.GetProcessString(), Proc.GetProcessString());
	ASSERT_EQ(Proc2.NumThreads(), Proc.NumThreads());
}

TEST(Scheduler, CreateThreadWithProcName)
{
	pantheon::Process Proc("./someprocess");
	pantheon::Thread T(&Proc);
	ASSERT_EQ(T.MyProc(), &Proc);
	ASSERT_EQ(Proc.GetProcessString(), "./someprocess");
}

TEST(Scheduler, CreateThreadWithProcNameKernelString)
{
	pantheon::String MyStr("./someprocess");
	pantheon::Process Proc(MyStr);
	pantheon::Thread T(&Proc);
	ASSERT_EQ(T.MyProc(), &Proc);
	ASSERT_EQ(Proc.GetProcessString(), MyStr);
}

TEST(Scheduler, CreateThreadPriority)
{
	pantheon::Process Proc;
	pantheon::Thread T(&Proc);
	ASSERT_EQ(T.MyPriority(), pantheon::THREAD_PRIORITY_NORMAL);
}

TEST(Scheduler, CreateThreadChangePriority)
{
	pantheon::Process Proc;
	pantheon::Thread T(&Proc);
	T.SetPriority(pantheon::THREAD_PRIORITY_VERYLOW);
	ASSERT_EQ(T.MyPriority(), pantheon::THREAD_PRIORITY_VERYLOW);
	T.SetPriority(pantheon::THREAD_PRIORITY_VERYHIGH);
	ASSERT_EQ(T.MyPriority(), pantheon::THREAD_PRIORITY_VERYLOW);
}

TEST(Scheduler, CreateThreadHighPriority)
{
	pantheon::Process Proc;
	pantheon::Thread T(&Proc, pantheon::THREAD_PRIORITY_VERYHIGH);
	ASSERT_EQ(T.MyPriority(), pantheon::THREAD_PRIORITY_VERYHIGH);
}

TEST(Scheduler, CreateThreadTicks)
{
	pantheon::Process Proc;
	pantheon::Thread T1(&Proc, pantheon::THREAD_PRIORITY_VERYLOW);
	pantheon::Thread T2(&Proc, pantheon::THREAD_PRIORITY_VERYHIGH);
	ASSERT_EQ(T2.TicksLeft(), (T2.MyPriority() + 1) * T1.TicksLeft());
}

TEST(Scheduler, CreateThreadInitState)
{
	pantheon::Process Proc;
	pantheon::Thread T(&Proc);
	ASSERT_EQ(T.MyState(), pantheon::THREAD_STATE_INIT);
}

TEST(Scheduler, CreateThreadRunState)
{
	pantheon::Process Proc;
	pantheon::Thread T(&Proc);
	T.SetState(pantheon::THREAD_STATE_RUNNING);
	ASSERT_EQ(T.MyState(), pantheon::THREAD_STATE_RUNNING);
}

TEST(Scheduler, CreateThreadPreempts)
{
	pantheon::Process Proc;
	pantheon::Thread T(&Proc);
	ASSERT_EQ(T.Preempts(), 0);
}


TEST(Scheduler, CreateThreadAddTicks)
{
	pantheon::Process Proc;
	pantheon::Thread T(&Proc);
	UINT64 OrigTicks = T.TicksLeft();
	T.AddTicks(100);
	ASSERT_EQ(T.TicksLeft(), OrigTicks + 100);
}

TEST(Scheduler, CreateThreadSelfAssign)
{
	pantheon::Process Proc;
	pantheon::Thread T(&Proc);
	UINT64 OrigTicks = T.TicksLeft();
	T.AddTicks(100);
	pantheon::Thread T2 = T;
	ASSERT_EQ(T.TicksLeft(), OrigTicks + 100);
	ASSERT_EQ(T2.TicksLeft(), T.TicksLeft());
}

TEST(Scheduler, CreateThreadAddTicksCopy)
{
	pantheon::Process Proc;
	pantheon::Thread T(&Proc);
	UINT64 OrigTicks = T.TicksLeft();
	T.AddTicks(100);
	pantheon::Thread T2 = T;
	ASSERT_EQ(T2.MyProc(), T.MyProc());
	ASSERT_EQ(T2.TicksLeft(), T.TicksLeft());
	ASSERT_EQ(T2.Preempts(), T.Preempts());
	ASSERT_EQ(T2.MyPriority(), T.MyPriority());
	ASSERT_EQ(T2.MyState(), T.MyState());
	ASSERT_EQ(T2.TicksLeft(), OrigTicks + 100);
}

TEST(Scheduler, CreateThreadWithDeepCopy)
{
	pantheon::Process Proc;
	pantheon::Thread T(&Proc);
	UINT64 OrigTicks = T.TicksLeft();
	T.AddTicks(100);
	pantheon::Thread T2(T);
	T2.AddTicks(100);
	ASSERT_EQ(T2.MyProc(), T.MyProc());
	ASSERT_EQ(T2.TicksLeft(), OrigTicks + 200);
	ASSERT_EQ(T2.Preempts(), T.Preempts());
	ASSERT_EQ(T2.MyPriority(), T.MyPriority());
	ASSERT_EQ(T2.MyState(), T.MyState());
	ASSERT_EQ(T2.TicksLeft(), T.TicksLeft() + 100);
	T.AddTicks(100);
	T.GetRegisters()[3] = 100;
	ASSERT_NE(T2.GetRegisters()[3], T.GetRegisters()[3]);
	ASSERT_EQ(T2.TicksLeft(), T.TicksLeft());
}

TEST(Scheduler, CreateThreadWithConstCopy)
{
	pantheon::Process Proc;
	const pantheon::Thread T(&Proc);
	pantheon::Thread T2 = T;
	T2.AddTicks(100);
	ASSERT_NE(T.TicksLeft(), T2.TicksLeft());
	ASSERT_EQ(T.TicksLeft() + 100, T2.TicksLeft());
}

TEST(Scheduler, CreateThreadAddTicksDeepCopy)
{
	pantheon::Process Proc;
	pantheon::Thread T(&Proc);
	UINT64 OrigTicks = T.TicksLeft();
	T.AddTicks(100);
	pantheon::Thread T2 = T;
	T2.AddTicks(100);
	ASSERT_EQ(T2.MyProc(), T.MyProc());
	ASSERT_NE(T2.TicksLeft(), T.TicksLeft());
	ASSERT_EQ(T2.Preempts(), T.Preempts());
	ASSERT_EQ(T2.MyPriority(), T.MyPriority());
	ASSERT_EQ(T2.MyState(), T.MyState());
	ASSERT_EQ(T.TicksLeft(), OrigTicks + 100);
	ASSERT_NE(T2.TicksLeft(), OrigTicks + 100);
}

TEST(Scheduler, SchedulerNoThreadsNoSchedule)
{
	pantheon::Scheduler Sched;
	Sched.Reschedule();
	ASSERT_EQ(Sched.MyThread(), nullptr);
	ASSERT_EQ(Sched.MyProc(), nullptr);
}

TEST(Scheduler, DefaultProcessName)
{
	pantheon::Process Proc;
	ASSERT_EQ(Proc.GetProcessString(), pantheon::String("kernel"));
}

TEST(Scheduler, ManyProcessIDs)
{
	pantheon::Process OneProc;
	pantheon::Process TwoProc;
	pantheon::Process OtherProc;
	ASSERT_TRUE(OneProc.ProcessID() < OtherProc.ProcessID());
}

TEST(Scheduler, ManyThreadIDs)
{
	pantheon::Thread OneProc(nullptr);
	pantheon::Thread TwoProc(nullptr);
	pantheon::Thread OtherProc(nullptr);
	ASSERT_TRUE(OneProc.ThreadID() < OtherProc.ThreadID());
}

TEST(CPUCore, CoreInfoInit)
{
	pantheon::CPU::InitCoreInfo(0);
	ASSERT_EQ(pantheon::CPU::GetCoreInfo()->CurState, pantheon::CPU::CPU_STATE_IDLE);
	ASSERT_EQ(pantheon::CPU::GetCoreInfo()->CurProcess, nullptr);
	ASSERT_EQ(pantheon::CPU::GetCoreInfo()->CurThread, nullptr);
	ASSERT_NE(pantheon::CPU::GetCoreInfo()->CurSched, nullptr);
}

TEST(Scheduler, GlobalSchedulerCreateProcess)
{
	pantheon::GlobalScheduler Sched;
	Sched.CreateProcess("some proc", nullptr);
	Optional<pantheon::Process> Proc = Sched.AcquireProcess();
	ASSERT_TRUE(Proc.GetOkay());
	ASSERT_EQ(Proc().NumThreads(), 1);
}

TEST(Scheduler, AcquireProcessWhileNoneReady)
{
	pantheon::GlobalScheduler Sched;
	Optional<pantheon::Process> NotProc = Sched.AcquireProcess();
	ASSERT_FALSE(NotProc.GetOkay());
}

TEST(Scheduler, GlobalSchedulerExists)
{
	ASSERT_NE(pantheon::GetGlobalScheduler(), nullptr);
}

#endif