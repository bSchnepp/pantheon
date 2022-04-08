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

	class Port;
	class ClientPort;

	class Connection;
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
	pantheon::ipc::ReadableEvent *ReadEvent;
	pantheon::ipc::WritableEvent *WriteEvent;
	pantheon::Process *Process;
	pantheon::Thread *Thread;
	pantheon::ipc::Port *Port;
	pantheon::ipc::ClientPort *ClientPort;
	pantheon::ipc::Connection *Connection;
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
	Handle(pantheon::ipc::Port *Port);
	Handle(pantheon::ipc::ClientPort *ClientPort);
	Handle(pantheon::ipc::Connection *Connection);
	Handle(pantheon::ipc::ClientConnection *ClientConnection);
	~Handle() = default;

	HandleType GetType();
	HandleContent &GetContent();

	[[nodiscard]] bool IsValid() const
	{
		return this->Type != pantheon::HANDLE_TYPE_INVALID;
	}

private:
	HandleType Type;
	HandleContent Content;
};

}

#endif