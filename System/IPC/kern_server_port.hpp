#include <kern_object.hpp>
#include <kern_datatypes.hpp>

#include <Common/Sync/kern_atomic.hpp>
#include <Common/Sync/kern_lockable.hpp>

#include <Common/Structures/kern_linkedlist.hpp>
#include <Common/Structures/kern_allocatable.hpp>

#ifndef _KERN_SERVER_PORT_HPP_
#define _KERN_SERVER_PORT_HPP_

namespace pantheon::ipc
{

class Port;
class ServerConnection;

class ServerPort : public pantheon::Allocatable<ServerPort, 1024>, public Lockable, public pantheon::Object
{
public:
	explicit ServerPort() = default;

	void Initialize(Port *Owner);
	[[nodiscard]] Port *GetOwner() const { return this->Owner; }

	void Enqueue(ServerConnection *Conn);
	ServerConnection *Dequeue();

	void Cleanup();
	void DestroyObject() override;
	
private:
	pantheon::ipc::Port *Owner;
	pantheon::LinkedList<ServerConnection> ConnectionList;
};

}

#endif