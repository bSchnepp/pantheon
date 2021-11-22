#include <System/IPC/kern_event.hpp>

#ifndef _KERN_HANDLE_HPP_
#define _KERN_HANDLE_HPP_

namespace pantheon
{

typedef enum HandleType
{
	HANDLE_TYPE_INVALID,
	HANDLE_TYPE_READ_SIGNAL,
	HANDLE_TYPE_WRITE_SIGNAL,
}HandleType;

typedef union HandleContent
{
	pantheon::ipc::ReadableEvent *ReadEvent;
	pantheon::ipc::WritableEvent *WriteEvent;
}HandleContent;

class Handle
{
public:
	Handle();
	Handle(pantheon::ipc::ReadableEvent *Evt);
	Handle(pantheon::ipc::WritableEvent *Evt);
	~Handle();

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