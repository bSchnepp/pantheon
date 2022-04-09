#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>
#include <System/IPC/kern_port.hpp>
#include <System/IPC/kern_client_port.hpp>

/**
 * @file System/IPC/kern_port.cpp
 * @brief Description and definition of a pantheon port.
 */

pantheon::ipc::Port::Port() : pantheon::Lockable("Port")
{
	this->Name.AsNumber = 0;
	this->MaxConnectionCount = -1;
}

pantheon::ipc::Port::~Port()
{

}

void pantheon::ipc::Port::Initialize(PortName Name, INT64 MaxConnections)
{
	this->MaxConnectionCount = MaxConnections;
	this->Name = Name;

	this->Client = pantheon::ipc::ClientPort::Create();
	this->Client->Initialize(this, MaxConnections);
}

void pantheon::ipc::Port::CloseServerHandler()
{
	OBJECT_SELF_ASSERT();
	pantheon::ScopedLock _L(this);
	if (this->CurrentState == pantheon::ipc::Port::State::OPEN)
	{
		this->CurrentState = pantheon::ipc::Port::State::CLOSED_SERVER;
	}
}

void pantheon::ipc::Port::CloseClientHandler()
{
	OBJECT_SELF_ASSERT();
	pantheon::ScopedLock _L(this);
	if (this->CurrentState == pantheon::ipc::Port::State::OPEN)
	{
		this->CurrentState = pantheon::ipc::Port::State::CLOSED_CLIENT;
	}
}