#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>
#include <System/IPC/kern_port.hpp>
#include <System/IPC/kern_client_port.hpp>
#include <System/IPC/kern_server_port.hpp>

#include <System/Proc/kern_sched.hpp>

/**
 * @file System/IPC/kern_port.cpp
 * @brief Description and definition of a pantheon port.
 */

pantheon::ipc::Port::Port() : pantheon::Lockable("Port")
{
	this->Name.AsNumber = 0;
}

pantheon::ipc::Port::~Port()
{

}

void pantheon::ipc::Port::Initialize(PortName Name, INT64 MaxConnections)
{
	this->Name = Name;

	this->Client = pantheon::ipc::ClientPort::Create();
	this->Server = pantheon::ipc::ServerPort::Create();

	this->Server->Initialize(this);
	this->Client->Initialize(this, MaxConnections);

	this->CurrentState = pantheon::ipc::Port::State::OPEN;


}

void pantheon::ipc::Port::CloseServerHandler()
{
	OBJECT_SELF_ASSERT();
	pantheon::ScopedGlobalSchedulerLock SchedLock;
	if (this->CurrentState == pantheon::ipc::Port::State::OPEN)
	{
		this->CurrentState = pantheon::ipc::Port::State::CLOSED_SERVER;
	}
	else if (this->CurrentState == pantheon::ipc::Port::State::CLOSED_CLIENT)
	{
		this->CurrentState = pantheon::ipc::Port::State::CLOSED;
	}
}

void pantheon::ipc::Port::CloseClientHandler()
{
	OBJECT_SELF_ASSERT();
	pantheon::ScopedGlobalSchedulerLock SchedLock;
	if (this->CurrentState == pantheon::ipc::Port::State::OPEN)
	{
		this->CurrentState = pantheon::ipc::Port::State::CLOSED_CLIENT;
	}
	else if (this->CurrentState == pantheon::ipc::Port::State::CLOSED_SERVER)
	{
		this->CurrentState = pantheon::ipc::Port::State::CLOSED;
	}
}

pantheon::Result pantheon::ipc::Port::Enqueue(pantheon::ipc::ServerConnection *Conn)
{
	pantheon::ScopedGlobalSchedulerLock SchedLock;
	if (this->CurrentState != pantheon::ipc::Port::State::OPEN)
	{
		return pantheon::Result::KERN_PORT_CLOSED;
	}
	this->Server->Enqueue(Conn);
	return pantheon::Result::SYS_OK;
}