#include <kern_datatypes.hpp>

#include <Common/Sync/kern_atomic.hpp>
#include <Common/Sync/kern_lockable.hpp>
#include <Common/Structures/kern_allocatable.hpp>

#include <kern_object.hpp>
#include <System/IPC/kern_synchronization.hpp>

void pantheon::Synchronization::Initialize(Condition C, CallbackFn F, VOID *Userdata)
{
    this->Userdata = Userdata;
    this->Fn = C;
    this->TrueFn = F;
}

BOOL pantheon::Synchronization::Trigger()
{
    if (this->Fn(this, Userdata))
    {
        this->TrueFn(this);
        return TRUE;
    }
    return FALSE;
}