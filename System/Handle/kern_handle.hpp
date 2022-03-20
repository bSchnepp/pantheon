#include <System/IPC/kern_event.hpp>

#ifndef _KERN_HANDLE_HPP_
#define _KERN_HANDLE_HPP_

namespace pantheon
{

/* Forward declarations */
class Process;
class Thread;

typedef enum HandleType
{
	HANDLE_TYPE_INVALID,
	HANDLE_TYPE_READ_SIGNAL,
	HANDLE_TYPE_WRITE_SIGNAL,
	HANDLE_TYPE_PROCESS,
	HANDLE_TYPE_THREAD,
}HandleType;

typedef union HandleContent
{
	pantheon::ipc::ReadableEvent *ReadEvent;
	pantheon::ipc::WritableEvent *WriteEvent;
	pantheon::Process *Process;
	pantheon::Thread *Thread;
}HandleContent;

class Handle
{
public:
	Handle();
	Handle(pantheon::ipc::ReadableEvent *Evt);
	Handle(pantheon::ipc::WritableEvent *Evt);
	Handle(pantheon::Process *Proc);
	Handle(pantheon::Thread *Thr);
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