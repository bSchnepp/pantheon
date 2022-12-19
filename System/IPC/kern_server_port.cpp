#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#include <System/IPC/kern_port.hpp>
#include <System/IPC/kern_connection.hpp>
#include <System/IPC/kern_server_port.hpp>
#include <System/IPC/kern_server_connection.hpp>

void pantheon::ipc::ServerPort::Initialize(pantheon::ipc::Port *Owner)
{
	this->Owner = Owner;
}

void pantheon::ipc::ServerPort::Cleanup()
{
	for (;;)
	{
		pantheon::Spinlock ConnectionLock("Connection lock");
		pantheon::ipc::ServerConnection *CurCon = nullptr;
		ConnectionLock.Acquire();
		if (this->ConnectionList.Size() != 0)
		{
			CurCon = this->ConnectionList.PopFront();
		}
		ConnectionLock.Release();

		if (CurCon)
		{
			CurCon->Close();
		}
		else
		{
			break;
		}
	}
}

void pantheon::ipc::ServerPort::Enqueue(pantheon::ipc::ServerConnection *Conn)
{
	this->ConnectionList.PushBack(Conn);
}

pantheon::ipc::ServerConnection *pantheon::ipc::ServerPort::Dequeue()
{
	/* NYI */
	if (this->ConnectionList.Size())
	{
		return this->ConnectionList.PopFront();
	}
	return nullptr;
}