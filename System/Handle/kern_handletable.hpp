#include <kern_datatypes.hpp>
#include <System/Handle/kern_handle.hpp>

#ifndef _KERN_HANDLE_TABLE_HPP_
#define _KERN_HANDLE_TABLE_HPP_

namespace pantheon
{


class HandleTable
{
public:
	HandleTable() = default;

	INT32 Create(const Handle &Item);
	pantheon::Handle *Get(INT32 Index);
	BOOL Release(INT32 Index);

	VOID Clear();

private:
	static constexpr INT32 HandleTableSize = 64;
	pantheon::Handle ProcHandleTable[HandleTableSize];	
};


}

#endif