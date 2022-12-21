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

BOOL pantheon::HandleTable::Release(INT32 TIndex)
{
	if (TIndex < 0)
	{
		return FALSE;
	}

	for (INT32 Index = 0; Index < HandleTable::HandleTableSize; Index++)
	{
		if (this->ProcHandleTable[Index].IsValid() == TRUE && TIndex == Index)
		{
			this->ProcHandleTable[Index].Close();
			return TRUE;
		}
	}
	return FALSE;
}

VOID pantheon::HandleTable::Clear()
{
	for (pantheon::Handle &Hand : this->ProcHandleTable)
	{
		Hand = pantheon::Handle();
	}
}