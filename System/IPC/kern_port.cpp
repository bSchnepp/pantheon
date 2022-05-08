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

static pantheon::LinkedList<pantheon::ipc::Port> NamedPortsList;
void pantheon::ipc::Port::Setup()
{
	NamedPortsList = LinkedList<pantheon::ipc::Port>();
}

static void Register(pantheon::ipc::Port *Current)
{
	/* Make sure this exists precisely once in the list? */
	NamedPortsList.PushBack(Current);
}

static void Unregister(pantheon::ipc::Port *Current)
{
	PANTHEON_UNUSED(Current);
	/* NYI */
}

void pantheon::ipc::Port::Initialize(PortName Name, INT64 MaxConnections)
{
	this->Name = Name;

	this->Client = pantheon::ipc::ClientPort::Create();
	this->Server = pantheon::ipc::ServerPort::Create();

	this->Server->Initialize(this);
	this->Client->Initialize(this, MaxConnections);

	this->CurrentState = pantheon::ipc::Port::State::OPEN;

	if (Name.AsNumber != 0)
	{
		Register(this);
	}
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
		Unregister(this);
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
		Unregister(this);
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