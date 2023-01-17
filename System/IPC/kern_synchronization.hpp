#include <kern_datatypes.hpp>

#include <Common/Sync/kern_atomic.hpp>
#include <Common/Sync/kern_lockable.hpp>
#include <Common/Structures/kern_allocatable.hpp>

#include <kern_object.hpp>

#ifndef _KERN_SYNCHRONIZATION_HPP_
#define _KERN_SYNCHRONIZATION_HPP_

namespace pantheon
{

class Synchronization : public pantheon::Object<Synchronization, 2048>
{
public:
    typedef BOOL (*Condition)(Synchronization *Self, VOID *Userdata);
    typedef void (*CallbackFn)(Synchronization *Self);

public:
    Synchronization() = default;
    ~Synchronization() = default;

    void Initialize(Condition C, CallbackFn F, VOID *Userdata);
    BOOL Trigger();

private:
    Condition Fn;
    CallbackFn TrueFn;
    VOID *Userdata;

};

}
#endif