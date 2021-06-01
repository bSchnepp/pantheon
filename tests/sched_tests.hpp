#include <gtest/gtest.h>

#include <kern_runtime.hpp>
#include <kern_container.hpp>

#include <Common/Proc/kern_sched.hpp>

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

#endif