#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#ifndef _KERN_CONTAINER_HPP_
#define _KERN_CONTAINER_HPP_

template<typename T>
class ArrayList
{
public:
	ArrayList() : ArrayList(2){};

	ArrayList(UINT64 InitCount)
	{
		auto MaybeMem = BasicMalloc(sizeof(T) * InitCount);
		if (MaybeMem.GetOkay() != FALSE)
		{
			this->Content = (T*)MaybeMem.GetValue();
			this->SpaceCount = InitCount;
			this->EntryCount = 0;
		}
	}

	Optional<T> operator[](UINT64 Index)
	{
		if (Index < EntryCount)
		{
			return Optional<T>(Content[Index]);
		}
		return Optional<T>();
	}

	void Add(T& NewItem)
	{
		if (EntryCount + 1 < SpaceCount)
		{
			this->Content[this->EntryCount] = NewItem;
			this->EntryCount++;
		}
		else
		{
			this->SpaceCount *= 2;
			auto MaybeMem = BasicMalloc(sizeof(T) * this->SpaceCount);
			if (MaybeMem.GetOkay() != FALSE)
			{
				T* NewContent = (T*)MaybeMem.GetValue();
				for (UINT64 Index = 0; Index < this->EntryCount; ++Index)
				{
					NewContent[Index] = this->Content[Index];
				}
				BasicFree(this->Content);
				this->Content = NewContent;
				this->Add(NewItem);
			}			
		}
	}

	void Delete(UINT64 Index)
	{
		/* NYI */
	}

	BOOL Contains(T& Item)
	{
		/* NYI */
	}

private:
	UINT64 SpaceCount;
	UINT64 EntryCount;
	T *Content;
};


#endif