#include <kern_string.hpp>

#include <Common/kern_container.hpp>
#include <Common/Structures/kern_slab.hpp>
#include <System/IPC/kern_event.hpp>

/* TODO: Make allocator more robust */
static constexpr UINT64 NumEvents = 64; 
CHAR AreaBuffer[NumEvents * sizeof(pantheon::ipc::NamedEvent)];
CHAR WritableBuffer[NumEvents * sizeof(pantheon::ipc::NamedEvent)];
CHAR ReadableBuffer[NumEvents * sizeof(pantheon::ipc::NamedEvent)];

pantheon::mm::SlabCache<pantheon::ipc::NamedEvent> NamedEventAllocator;
pantheon::mm::SlabCache<pantheon::ipc::WritableEvent> WritableEventAllocator;
pantheon::mm::SlabCache<pantheon::ipc::ReadableEvent> ReadableEventAllocator;

struct NamedEventContainer
{
	pantheon::ipc::NamedEvent* Ptr;
	BOOL Valid;
};

static ArrayList<NamedEventContainer> ValidEvents;

void pantheon::ipc::InitEventSystem()
{
	ValidEvents = ArrayList<NamedEventContainer>(NumEvents);
	NamedEventAllocator = pantheon::mm::SlabCache<pantheon::ipc::NamedEvent>(AreaBuffer);
	WritableEventAllocator = pantheon::mm::SlabCache<pantheon::ipc::WritableEvent>(WritableBuffer);
	ReadableEventAllocator = pantheon::mm::SlabCache<pantheon::ipc::ReadableEvent>(ReadableBuffer);
}

pantheon::ipc::NamedEvent *pantheon::ipc::CreateNamedEvent(const pantheon::String &Name, pantheon::Process *Creator)
{
	pantheon::ipc::NamedEvent *Evt = NamedEventAllocator.Allocate();
	if (Evt)
	{
		Evt->Creator = Creator;
		Evt->Name = Name;
		Evt->Readable = ReadableEventAllocator.Allocate();
		Evt->Writable = WritableEventAllocator.Allocate();
		Evt->Status = pantheon::ipc::EVENT_TYPE_UNSIGNALED;

		Evt->Writable->Parent = Evt;
		Evt->Readable->Parent = Evt;

		if (Creator == nullptr || Evt->Readable == nullptr || Evt->Writable == nullptr)
		{
			if (Evt->Readable)
			{
				ReadableEventAllocator.Deallocate(Evt->Readable);
			}

			if (Evt->Writable)
			{
				WritableEventAllocator.Deallocate(Evt->Writable);
			}
			NamedEventAllocator.Deallocate(Evt);
			return nullptr;
		}

		ValidEvents.Add({Evt, TRUE});
	}
	return Evt;

}

pantheon::ipc::NamedEvent *pantheon::ipc::LookupEvent(const pantheon::String &Name)
{
	for (auto &EvtItem : ValidEvents)
	{
		pantheon::ipc::NamedEvent *Evt = EvtItem.Ptr;
		if (Evt->Name == Name)
		{
			return Evt;
		}
	}
	return nullptr;
}

void pantheon::ipc::DestroyNamedEvent(pantheon::ipc::NamedEvent *Evt)
{
	if (Evt)
	{
		UINT64 SIndex = -1;
		for (UINT64 Index = 0; Index < ValidEvents.Size(); Index++)
		{
			if (ValidEvents[Index].Ptr == Evt)
			{
				SIndex = Index;
				break;
			}
		}

		if (SIndex >= 0)
		{
			ValidEvents.Delete(SIndex);
		}

		/* In order for this to be valid, all contents must also be
		 * valid. Therefore, the sub-contents must be deallocated first.
		 */
		ReadableEventAllocator.Deallocate(Evt->Readable);
		WritableEventAllocator.Deallocate(Evt->Writable);
		NamedEventAllocator.Deallocate(Evt);
	}
}