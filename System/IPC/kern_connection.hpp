#include <kern_datatypes.hpp>

#include <System/Proc/kern_proc.hpp>

#include <Common/Sync/kern_atomic.hpp>
#include <Common/Structures/kern_allocatable.hpp>

#ifndef _KERN_CONNECTION_HPP_
#define _KERN_CONNECTION_HPP_

namespace pantheon::ipc
{

class Port;

class Connection
{
public:
	explicit Connection();

	FORCE_INLINE BOOL IsServerClosed()
	{
		return this->CurState == State::CLOSED_SERVER;
	}

	FORCE_INLINE BOOL IsClientClosed()
	{
		return this->CurState == State::CLOSED_CLIENT;
	}

	void CloseServerHandler();
	void CloseClientHandler();



private:
	enum class State : UINT8
	{
		UNUSED = 0,
		OPEN = 1,
		CLOSED_CLIENT = 3,
		CLOSED_SERVER = 4,
	};

private:
	pantheon::Atomic<State> CurState;
};

}

#endif