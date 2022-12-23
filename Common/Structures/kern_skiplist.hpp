#include <kern_rand.hpp>
#include <kern_datatypes.hpp>

#include <Common/Structures/kern_optional.hpp>
#include <Common/Structures/kern_allocatable.hpp>

#ifndef _KERN_SKIPLIST_HPP_
#define _KERN_SKIPLIST_HPP_

namespace pantheon
{

template <typename K, typename V>
class SkipList
{
public:
	SkipList(UINT64 MaxLvl = 8) : Head(nullptr), MaxLevel(MaxLvl), Sz(0) {}
	~SkipList() = default;

	[[nodiscard]] UINT64 Size() const
	{
		return this->Sz;
	}

	[[nodiscard]] BOOL Contains(K Key) const
	{
		return this->Get(Key).GetOkay();
	}

	[[nodiscard]] Optional<V> Get(K Key) const
	{
		KVPair *Cur = this->Head;
		while (Cur)
		{
			for (; Cur->Next && Cur->Next->Key < Key; Cur = Cur->Next) { }
			if (Cur->Next && Cur->Next->Key == Key)
			{
				return Optional<V>(Cur->Value);
			}
			Cur = Cur->Down;
		}
		return Optional<V>();
	}

	[[nodiscard]] Optional<K> MinKey() const
	{
		if (this->Head == nullptr)
		{
			return Optional<K>();
		}

		KVPair *Cur;
		for (Cur = this->Head; Cur->Down != nullptr; Cur = Cur->Down) { }
		return Optional<K>(Cur->Key);
	}

	[[nodiscard]] Optional<K> MaxKey() const
	{
		if (this->Head == nullptr)
		{
			return Optional<K>();
		}

		KVPair *Cur = this->Head;
		while (Cur->Right || Cur->Down)
		{
			/* Go right as far as possible */
			if (Cur->Right)
			{
				Cur = Cur->Right;
			} else {
				/* Go down if we have to*/
				Cur = Cur->Down;
			}
		}

		/* If neither, we found the max value. */
		return Optional<K>(Cur->Key);
	}

	VOID Insert(K Key, V Value)
	{
		if (this->Head == nullptr)
		{
			this->Head = KVPair::Create();
			this->Head->Key = Key;
			this->Head->Value = Value;
			this->Head->Next = nullptr;
			this->Head->Down = nullptr;
			return;	
		}

		KVPair *Cur = this->Head;
		KVPair *Prev = nullptr;

		/* Iterate to the bottom */
		while (Cur)
		{
			for (; Cur->Next && Cur->Next->Key < Key; Cur = Cur->Next) { }
			Prev = Cur;
			Cur = Cur->Down;
		}

		/* Put a new node at the bottom */
		Cur = Prev;
		UINT64 Level = this->RandLevel();

		/* At every level, create a new KVPair */
		for (UINT64 Index = 0; Index < Level; Index++)
		{
			KVPair *Node = KVPair::Create();
			Node->Key = Key;
			Node->Value = Value;
			Node->Next = Cur->Next;
			Node->Down = nullptr;

			Cur->Next = Node;

			if (Index < Level - 1)
			{
				Head = Node;
				Cur = Head;
			}
		}
		this->Sz++;
	}

	BOOL Delete(K Key)
	{
		if (this->Head == nullptr)
		{
			return FALSE;
		}

		KVPair *Cur = this->Head;
		KVPair *Prev = nullptr;

		while (Cur)
		{
			for (; Cur->Next && Cur->Next->Key < Key; Cur = Cur->Next) { }
			Prev = Cur;
			Cur = Cur->Down;
			if (Prev->Next && Prev->Next->Key == Key)
			{
				Prev->Next = Cur->Next;
				KVPair::Destroy(Cur);
				this->Sz--;
				return TRUE;
			}
		}
		return FALSE;
	}


	V& operator[](K Key)
	{
		KVPair *Cur = this->Head;
		while (Cur)
		{
			for (; Cur->Next && Cur->Next->Key < Key; Cur = Cur->Next) { }
			if (Cur->Next && Cur->Next->Key == Key)
			{
				return Cur->Value;
			}
			Cur = Cur->Down;
		
		}
		static V DefaultItem;
		return DefaultItem;
	}

private:
	struct KVPair : public Allocatable<struct KVPair, 4096>
	{
		K Key;
		V Value;
		KVPair *Next;
		KVPair *Down;
	};

	KVPair *Head;
	UINT64 MaxLevel;
	UINT64 Sz;

	UINT64 RandLevel()
	{
		UINT64 Level = 1;

		/* Rand() generates a 64-bit number.
		 * The probability of any one bit in particular being 0 or 1
		 * is 50% for either case. We only care about one bit,
		 * so let's use the top bit for this.
		 */
		const UINT64 MAX_INT64 = 0x7FFFFFFFFFFFFFFF;
		while (pantheon::Rand() < MAX_INT64 && Level < MaxLevel)
		{
			Level++;
		}
		return Level;
	}
};

}
#endif