#include <kern_object.hpp>

#ifndef _KERN_HANDLE_HPP_
#define _KERN_HANDLE_HPP_

namespace pantheon
{

/* Forward declarations */
class Process;
class Thread;

namespace ipc
{
	struct ReadableEvent;
	struct WritableEvent;

	class ServerPort;
	class ClientPort;

	class ServerConnection;
	class ClientConnection;
}

typedef enum HandleType
{
	HANDLE_TYPE_INVALID,
	HANDLE_TYPE_READ_SIGNAL,
	HANDLE_TYPE_WRITE_SIGNAL,
	HANDLE_TYPE_PROCESS,
	HANDLE_TYPE_THREAD,
	HANDLE_TYPE_SERVER_PORT,
	HANDLE_TYPE_SERVER_CONNECTION,
	HANDLE_TYPE_CLIENT_PORT,
	HANDLE_TYPE_CLIENT_CONNECTION,
}HandleType;

typedef union HandleContent
{
	void *RawPtr;
	pantheon::ipc::ReadableEvent *ReadEvent;
	pantheon::ipc::WritableEvent *WriteEvent;
	pantheon::Process *Process;
	pantheon::Thread *Thread;
	pantheon::ipc::ServerPort *ServerPort;
	pantheon::ipc::ClientPort *ClientPort;
	pantheon::ipc::ServerConnection *Connection;
	pantheon::ipc::ClientConnection *ClientConnection;
}HandleContent;

class Handle
{
public:
	Handle();
	Handle(pantheon::ipc::ReadableEvent *Evt);
	Handle(pantheon::ipc::WritableEvent *Evt);
	Handle(pantheon::Process *Proc);
	Handle(pantheon::Thread *Thr);
	Handle(pantheon::ipc::ServerPort *Port);
	Handle(pantheon::ipc::ClientPort *ClientPort);
	Handle(pantheon::ipc::ServerConnection *Connection);
	Handle(pantheon::ipc::ClientConnection *ClientConnection);
	~Handle() = default;

	HandleType GetType();
	HandleContent &GetContent();

	[[nodiscard]] bool IsValid() const
	{
		return this->Type != pantheon::HANDLE_TYPE_INVALID;
	}

	template<typename T>
	T *GetPtr()
	{
		if (this->Type == HANDLE_TYPE_INVALID)
		{
			return nullptr;
		}
		return reinterpret_cast<T*>(this->Content.RawPtr);
	}

	void Close();

private:
	HandleType Type;
	HandleContent Content;
};

static constexpr INT32 INVALID_HANDLE_ID = -1;

}

#endif