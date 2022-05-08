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

typedef union PortAlias
{
	pantheon::ipc::Port *Ptr;
	pantheon::vmm::VirtualAddress VAddr;
}PortAlias;

static ArrayList<PortAlias> NamedPortsList;
void pantheon::ipc::Port::Setup()
{
	NamedPortsList = ArrayList<PortAlias>(1024);
}

static void Register(pantheon::ipc::Port *Current)
{
	/* Make sure this exists precisely once in the list? */
	NamedPortsList.Add({Current});
}

static void Unregister(pantheon::ipc::Port *Current)
{
	INT64 DelIndex = -1;
	for (UINT64 Index = 0; Index < NamedPortsList.Size(); ++Index)
	{
		if (NamedPortsList[Index].Ptr == Current)
		{
			DelIndex = (INT64)Index;
			break;
		}
	}

	if (DelIndex >= 0)
	{
		NamedPortsList.Delete(DelIndex);
	}
}

pantheon::ipc::Port *pantheon::ipc::Port::GetRegistered(const PortName &Name)
{
	if (Name.AsNumber == 0)
	{
		return nullptr;
	}

	for (const PortAlias &Item : NamedPortsList)
	{
		if (Item.Ptr->GetName().AsNumber == Name.AsNumber)
		{
			return Item.Ptr;
		}
	}

	return nullptr;
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