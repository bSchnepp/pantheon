#include <kern_datatypes.hpp>

#include <Common/Sync/kern_lockable.hpp>
#include <Common/Structures/kern_allocatable.hpp>

#ifndef _KERN_PORT_HPP_
#define _KERN_PORT_HPP_

namespace pantheon::ipc
{

static const UINT32 PortNameLength = 8;

typedef union PortName
{
	UINT64 AsNumber;
	CHAR AsChars[pantheon::ipc::PortNameLength];
}PortName;

class Port : public pantheon::Allocatable<Port, 128>, public Lockable
{
public:
	Port();
	~Port() override;

	void Initialize(PortName Name, INT64 MaxConnections);
	void CloseServer();
	void CloseClient();

	/* TODO: Implement ServerBind(), ClientBind(), etc. */

private:
	enum class State : UINT8
	{
		UNUSED = 0,
		OPEN = 1,
		CLOSED_CLIENT = 3,
		CLOSED_SERVER = 4,
	};

private:
	PortName Name;
	State CurrentState;
	INT64 MaxConnectionCount;
};


static_assert(sizeof(PortName) == pantheon::ipc::PortNameLength);
}

#endif