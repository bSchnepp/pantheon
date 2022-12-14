#include <kern_string.hpp>
#include <kern_datatypes.hpp>
#include <Common/kern_object.hpp>

#ifndef _KERN_EVENT_HPP_
#define _KERN_EVENT_HPP_

namespace pantheon
{
class Process;
}

namespace pantheon::ipc
{

typedef enum EventStatus
{
	EVENT_TYPE_UNSIGNALED,
	EVENT_TYPE_SIGNALED,
}EventStatus;

struct WritableEvent;
struct ReadableEvent;

struct Event
{
	WritableEvent *Writable;
	ReadableEvent *Readable;
	pantheon::Process *Creator;
	volatile EventStatus Status;
};

struct NamedEvent : public Event
{
	pantheon::String Name;
};

typedef struct WritableEvent : public pantheon::Object
{
	Event *Parent;
	pantheon::Process *Signaler;

	void DestroyObject() override 
	{
		this->Parent = nullptr;
		this->Signaler = nullptr;
	}
}WritableEvent;

typedef struct ReadableEvent : public pantheon::Object
{
	Event *Parent;
	pantheon::Process *Clearer;

	void DestroyObject() override 
	{
		this->Parent = nullptr;
		this->Clearer = nullptr;
	}
}ReadableEvent;


void InitEventSystem();
NamedEvent *CreateNamedEvent(const pantheon::String &Name, pantheon::Process *Creator);
NamedEvent *LookupEvent(const pantheon::String &Name);
void DestroyNamedEvent(NamedEvent *Event);

}

#endif