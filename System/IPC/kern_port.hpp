#include <kern_datatypes.hpp>
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

class Port : public pantheon::Allocatable<Port, 128>
{
public:
	Port();
	virtual ~Port();
	
private:
	PortName Name;
};


static_assert(sizeof(PortName) == pantheon::ipc::PortNameLength);
}

#endif