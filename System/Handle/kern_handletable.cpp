#include <System/Handle/kern_handle.hpp>
#include <System/Handle/kern_handletable.hpp>


INT32 pantheon::HandleTable::Create(const pantheon::Handle &Item)
{
	for (UINT64 Index = 0; Index < HandleTable::HandleTableSize; Index++)
	{
		if (this->ProcHandleTable[Index].IsValid() == FALSE)
		{
			this->ProcHandleTable[Index] = Item;
			return static_cast<INT32>(Index);
		}
	}
	return -1;	
}

pantheon::Handle *pantheon::HandleTable::Get(INT32 GIndex)
{
	if (GIndex < 0)
	{
		return nullptr;
	}

	for (INT32 Index = 0; Index < HandleTable::HandleTableSize; Index++)
	{
		if (this->ProcHandleTable[Index].IsValid() == TRUE && GIndex == Index)
		{
			return &(this->ProcHandleTable[Index]);
		}
	}
	return nullptr;
}