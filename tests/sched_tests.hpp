#include <gtest/gtest.h>

#include <kern_runtime.hpp>
#include <kern_container.hpp>

#include <Proc/kern_cpu.hpp>
#include <Proc/kern_proc.hpp>
#include <Proc/kern_sched.hpp>
#include <Proc/kern_thread.hpp>

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

TEST(Scheduler, CreateThreadWithProcName)
{
	pantheon::Process Proc;
	pantheon::ProcessCreateInfo Info = {nullptr};
	Info.Name = "./someprocess";
	Proc.Lock();
	Proc.Initialize(Info);
	Proc.Unlock();
	pantheon::Thread T(&Proc);
	ASSERT_EQ(T.MyProc(), &Proc);
	ASSERT_EQ(Proc.GetProcessString(), "./someprocess");
}

TEST(Scheduler, CreateThreadWithProcNameKernelString)
{
	pantheon::String MyStr("./someprocess");
	pantheon::Process Proc;
	pantheon::ProcessCreateInfo Info = {nullptr};
	Info.Name = MyStr;
	Proc.Lock();
	Proc.Initialize(Info);
	Proc.Unlock();
	pantheon::Thread T(&Proc);
	ASSERT_EQ(T.MyProc(), &Proc);
	ASSERT_EQ(Proc.GetProcessString(), MyStr);
}

TEST(Scheduler, CreateThreadPriority)
{
	pantheon::Process Proc;
	pantheon::Thread T(&Proc);
	T.Lock();
	ASSERT_EQ(T.MyPriority(), pantheon::Thread::PRIORITY_NORMAL);
	T.Unlock();
}

TEST(Scheduler, CreateThreadChangePriority)
{
	pantheon::Process Proc;
	pantheon::Thread T(&Proc);
	T.Lock();
	T.SetPriority(pantheon::Thread::PRIORITY_VERYLOW);
	ASSERT_EQ(T.MyPriority(), pantheon::Thread::PRIORITY_VERYLOW);
	T.SetPriority(pantheon::Thread::PRIORITY_VERYHIGH);
	ASSERT_EQ(T.MyPriority(), pantheon::Thread::PRIORITY_VERYLOW);
	T.Unlock();
}

TEST(Scheduler, CreateThreadHighPriority)
{
	pantheon::Process Proc;
	pantheon::Thread T(&Proc, pantheon::Thread::PRIORITY_VERYHIGH);
	T.Lock();
	ASSERT_EQ(T.MyPriority(), pantheon::Thread::PRIORITY_VERYHIGH);
	T.Unlock();
}

TEST(Scheduler, CreateThreadTicks)
{
	pantheon::Process Proc;
	pantheon::Thread T1(&Proc, pantheon::Thread::PRIORITY_VERYLOW);
	pantheon::Thread T2(&Proc, pantheon::Thread::PRIORITY_VERYHIGH);
	T1.Lock();
	T2.Lock();
	ASSERT_EQ(T2.TicksLeft(), (T2.MyPriority() + 1) * T1.TicksLeft());
	T1.Unlock();
	T2.Unlock();
}

TEST(Scheduler, CreateThreadInitState)
{
	pantheon::Process Proc;
	pantheon::Thread T(&Proc);
	T.Lock();
	ASSERT_EQ(T.MyState(), pantheon::Thread::STATE_INIT);
	T.Unlock();
}

TEST(Scheduler, CreateThreadRunState)
{
	pantheon::Process Proc;
	pantheon::Thread T(&Proc);
	T.Lock();
	T.SetState(pantheon::Thread::STATE_RUNNING);
	ASSERT_EQ(T.MyState(), pantheon::Thread::STATE_RUNNING);
	T.Unlock();
}


TEST(Scheduler, CreateThreadAddTicks)
{
	pantheon::Process Proc;
	pantheon::Thread T(&Proc);
	T.Lock();
	UINT64 OrigTicks = T.TicksLeft();
	T.AddTicks(100);
	ASSERT_EQ(T.TicksLeft(), OrigTicks + 100);
	T.Unlock();
}

TEST(Scheduler, CreateThreadSelfAssign)
{
	pantheon::Process Proc;
	pantheon::Thread T(&Proc);
	T.Lock();
	UINT64 OrigTicks = T.TicksLeft();
	T.AddTicks(100);
	pantheon::Thread T2 = T;
	T2.Lock();
	ASSERT_EQ(T.TicksLeft(), OrigTicks + 100);
	ASSERT_EQ(T2.TicksLeft(), T.TicksLeft());
	T2.Unlock();
	T.Unlock();
}

TEST(Scheduler, CreateThreadAddTicksCopy)
{
	pantheon::Process Proc;
	pantheon::Thread T(&Proc);
	T.Lock();
	UINT64 OrigTicks = T.TicksLeft();
	T.AddTicks(100);
	pantheon::Thread T2 = T;
	T2.Lock();
	ASSERT_EQ(T2.MyProc(), T.MyProc());
	ASSERT_EQ(T2.TicksLeft(), T.TicksLeft());
	ASSERT_EQ(T2.MyPriority(), T.MyPriority());
	ASSERT_EQ(T2.MyState(), T.MyState());
	ASSERT_EQ(T2.TicksLeft(), OrigTicks + 100);
	T2.Unlock();
	T.Unlock();
}

TEST(Scheduler, CreateThreadWithDeepCopy)
{
	pantheon::Process Proc;
	pantheon::Thread T(&Proc);
	UINT64 OrigTicks = T.TicksLeft();
	T.Lock();
	T.AddTicks(100);

	pantheon::Thread T2(T);
	T2.Lock();	
	T2.AddTicks(100);

	ASSERT_EQ(T2.MyProc(), T.MyProc());
	ASSERT_EQ(T2.TicksLeft(), OrigTicks + 200);
	ASSERT_EQ(T2.MyPriority(), T.MyPriority());
	ASSERT_EQ(T2.MyState(), T.MyState());
	ASSERT_EQ(T2.TicksLeft(), T.TicksLeft() + 100);
	T.AddTicks(100);
	(*T.GetRegisters())[3] = 100;
	ASSERT_NE((*T2.GetRegisters())[3], (*T.GetRegisters())[3]);
	ASSERT_EQ(T2.TicksLeft(), T.TicksLeft());
	T2.Unlock();
	T.Unlock();
}

TEST(Scheduler, CreateThreadWithConstCopy)
{
	pantheon::Process Proc;
	const pantheon::Thread T(&Proc);
	pantheon::Thread T2 = T;
	T2.Lock();
	T2.AddTicks(100);
	ASSERT_NE(T.TicksLeft(), T2.TicksLeft());
	ASSERT_EQ(T.TicksLeft() + 100, T2.TicksLeft());
	T2.Unlock();
}

TEST(Scheduler, CreateThreadAddTicksDeepCopy)
{
	pantheon::Process Proc;
	pantheon::Thread T(&Proc);
	T.Lock();
	UINT64 OrigTicks = T.TicksLeft();
	T.AddTicks(100);
	pantheon::Thread T2 = T;
	T2.Lock();
	T2.AddTicks(100);
	ASSERT_EQ(T2.MyProc(), T.MyProc());
	ASSERT_NE(T2.TicksLeft(), T.TicksLeft());
	ASSERT_EQ(T2.MyPriority(), T.MyPriority());
	ASSERT_EQ(T2.MyState(), T.MyState());
	ASSERT_EQ(T.TicksLeft(), OrigTicks + 100);
	ASSERT_NE(T2.TicksLeft(), OrigTicks + 100);
	T2.Unlock();
	T.Unlock();
}

TEST(Scheduler, DefaultProcessName)
{
	pantheon::Process Proc;
	ASSERT_EQ(Proc.GetProcessString(), pantheon::String("idle"));
}

TEST(Scheduler, ManyProcessIDs)
{
	pantheon::ProcessCreateInfo Info = {nullptr};
	Info.Name = "one";

	pantheon::Process OneProc;
	OneProc.Lock();
	OneProc.Initialize(Info);
	OneProc.Unlock();

	Info.Name = "two";
	pantheon::Process TwoProc;
	TwoProc.Lock();
	TwoProc.Initialize(Info);
	TwoProc.Unlock();

	Info.Name = "three";
	pantheon::Process ThreeProc;
	ThreeProc.Lock();
	ThreeProc.Initialize(Info);
	ThreeProc.Unlock();

	ASSERT_NE(OneProc.ProcessID(), 0);
	ASSERT_NE(OneProc.ProcessID(), TwoProc.ProcessID());
	ASSERT_NE(OneProc.ProcessID(), ThreeProc.ProcessID());

	ASSERT_NE(TwoProc.ProcessID(), 0);
	ASSERT_NE(TwoProc.ProcessID(), ThreeProc.ProcessID());

	ASSERT_NE(ThreeProc.ProcessID(), 0);
}

TEST(Scheduler, ManyThreadIDs)
{
	pantheon::Thread OneProc(nullptr);
	pantheon::Thread TwoProc(nullptr);
	pantheon::Thread OtherProc(nullptr);
	ASSERT_TRUE(OneProc.ThreadID() < OtherProc.ThreadID());
}

TEST(Scheduler, ProcessFromRawString)
{
	const char *SomeRawString = "some raw string";
	pantheon::ProcessCreateInfo Info = {nullptr};
	Info.Name = SomeRawString;

	pantheon::Process Proc;
	pantheon::Process Proc2;

	Proc.Lock();
	Proc2.Lock();

	Proc.Initialize(Info);
	Proc2.Initialize(Info);

	Proc.Unlock();
	Proc2.Unlock();

	ASSERT_EQ(Proc.GetProcessString(), pantheon::String(SomeRawString));
	ASSERT_NE(Proc.ProcessID(), Proc2.ProcessID());
}

#endif