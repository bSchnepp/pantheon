#include <kern_datatypes.hpp>

#include <Common/Sync/kern_atomic.hpp>
#include <Common/Sync/kern_lockable.hpp>
#include <Common/Structures/kern_allocatable.hpp>

#include <kern_object.hpp>

#ifndef _KERN_CLIENT_PORT_HPP_
#define _KERN_CLIENT_PORT_HPP_

namespace pantheon::ipc
{

class Port;
class ClientConnection;

class ClientPort : public pantheon::Allocatable<ClientPort, 1024>, public Lockable, public pantheon::Object
{
public:
	explicit ClientPort() = default;

	void Initialize(Port *Owner, INT64 MaxConnections);

	[[nodiscard]] Port *GetOwner() const { return this->Owner; }
	[[nodiscard]] INT64 GetConnectionCount() const { return this->ConnectionCount.Load(); }
	[[nodiscard]] INT64 GetMaxConnectionCount() const { return this->MaxConnectionCount; }

	[[nodiscard]] BOOL IsServerClosed() const;

	void DestroyObject() override;
	
private:
	pantheon::ipc::Port *Owner;

	INT64 MaxConnectionCount;
	pantheon::Atomic<INT64> ConnectionCount;
};

}


#endif