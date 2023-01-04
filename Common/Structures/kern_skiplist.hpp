#include <kern_rand.hpp>
#include <kern_datatypes.hpp>

#include <Common/Structures/kern_optional.hpp>
#include <Common/Structures/kern_allocatable.hpp>

#ifndef _KERN_SKIPLIST_HPP_
#define _KERN_SKIPLIST_HPP_

namespace pantheon
{

template <typename K, typename V, UINT8 MaxLvl = 8>
class SkipList
{
public:
	SkipList()
	{
		this->Head = nullptr;
		this->Level = 0;
		this->Sz = 0;
	}

	~SkipList()
	{
		/* NYI */
	}

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
		if (this->Head == nullptr)
		{
			return Optional<V>();
		}

		INT16 Lvl = this->Level;
		KVPair *Current = this->Head;
		KVPair *Next = Current;
		while (Lvl >= 0)
		{
			Next = Current->Next[Lvl];
			while (Next && (Next->Key <= Key))
			{
				Current = Next;
				Next = Current->Next[Lvl];
			}
			Lvl--;
		}

		if (Current->Key == Key)
		{
			return Optional<V>(Current->Value);
		}
		return Optional<V>();

	}

	[[nodiscard]] Optional<K> MinKey() const
	{
		if (this->Head == nullptr)
		{
			return Optional<K>();
		}

		KVPair *Current = this->Head->Next[0];
		if (Current && Current->Key != (K() - 1))
		{
			return Optional<K>(Current->Key);
		}
		return Optional<K>();
	}

	[[nodiscard]] Optional<K> MaxKey() const
	{
		if (this->Head == nullptr)
		{
			return Optional<K>();
		}

		KVPair *Current = this->Head->Prev[0];
		if (Current && Current->Key != (K() - 1))
		{
			return Optional<K>(Current->Key);
		}
		return Optional<K>();
	}

	VOID Insert(K Key, V Value)
	{
		this->Setup();

		KVPair *Update[MaxLvl];
		INT16 Lvl = this->Level;

		KVPair *Current = this->Head;
		KVPair *Next = Current;
		while (Lvl >= 0)
		{
			Next = Current->Next[Lvl];
			while (Next && (Next->Key <= Key))
			{
				Current = Next;
				Next = Current->Next[Lvl];
			}
			Update[Lvl] = Current;
			Lvl--;
		}

		/* This is where we'll need to be: this needs to match the current key. */
		if (Current->Key == Key)
		{
			Current->Value = Value;
			return;
		}

		/* Actually make the new entry */
		INT16 NewLvl = this->RandLevel();
		if (NewLvl > this->Level)
		{
			NewLvl = ++(this->Level);
			Update[NewLvl] = this->Head;
		}

		KVPair *NewNode = new KVPair();
		NewNode->Key = Key;
		NewNode->Value = Value;
		NewNode->Level = NewLvl;
		for (UINT8 Index = 0; Index < MaxLvl; Index++)
		{
			NewNode->Next[Index] = nullptr;
			NewNode->Prev[Index] = nullptr;
		}

		while (NewLvl >= 0)
		{
			Current = Update[NewLvl];

			NewNode->Next[NewLvl] = Current->Next[NewLvl];
			Current->Next[NewLvl] = NewNode;

			NewNode->Prev[NewLvl] = Current;
			NewNode->Next[NewLvl]->Prev[NewLvl] = NewNode;
			NewLvl--;
		}

		this->Sz++;

	}

	BOOL Delete(K Key)
	{
		this->Setup();

		KVPair *Update[MaxLvl];
		INT16 Lvl = this->Level;

		KVPair *Current = this->Head;
		KVPair *Next = Current;
		while (Lvl >= 0)
		{
			Next = Current->Next[Lvl];
			while (Next && (Next->Key <= Key))
			{
				Current = Next;
				Next = Current->Next[Lvl];
			}
			Update[Lvl] = Current;
			Lvl--;
		}

		/* This is where we'll need to be: this needs to match the current key. */
		if (Current->Key == Key)
		{
			UINT8 CurLvl = Current->Level;
			for (UINT8 Lvl = 0; Lvl <= CurLvl; Lvl++)
			{
				Current->Prev[Lvl]->Next[Lvl] = Current->Next[Lvl];
				Current->Next[Lvl]->Prev[Lvl] = Current->Prev[Lvl];
			}

			/* If this was top, we might have to fix levels. */
			if (CurLvl == this->Level)
			{
				while (this->Head->Next[CurLvl] == this->Head && this->Head->Prev[CurLvl] == this->Head)
				{
					if (CurLvl > 0)
					{
						CurLvl--;
					} else
					{
						break;
					}
					
				}
				this->Level = CurLvl;
			}

			this->Sz--;
			delete Current;
			return TRUE;
		}
		return FALSE;
	}

	Optional<V> Pop(K Key)
	{
		this->Setup();

		KVPair *Update[MaxLvl];
		INT16 Lvl = this->Level;

		KVPair *Current = this->Head;
		KVPair *Next = Current;
		while (Lvl >= 0)
		{
			Next = Current->Next[Lvl];
			while (Next && (Next->Key <= Key))
			{
				Current = Next;
				Next = Current->Next[Lvl];
			}
			Update[Lvl] = Current;
			Lvl--;
		}

		/* This is where we'll need to be: this needs to match the current key. */
		if (Current->Key == Key)
		{
			UINT8 CurLvl = Current->Level;
			for (UINT8 Lvl = 0; Lvl <= CurLvl; Lvl++)
			{
				Current->Prev[Lvl]->Next[Lvl] = Current->Next[Lvl];
				Current->Next[Lvl]->Prev[Lvl] = Current->Prev[Lvl];
			}

			/* If this was top, we might have to fix levels. */
			if (CurLvl == this->Level)
			{
				while (this->Head->Next[CurLvl] == this->Head && this->Head->Prev[CurLvl] == this->Head)
				{
					if (CurLvl > 0)
					{
						CurLvl--;
					} else
					{
						break;
					}
					
				}
				this->Level = CurLvl;
			}

			this->Sz--;
			if (Current->Key == Key)
			{
				return Optional<V>(Current->Value);
			}
			return Optional<V>();
		}
		return Optional<V>();
	}


	V& operator[](K Key)
	{
		this->Setup();

		INT16 Lvl = this->Level;
		KVPair *Current = this->Head;
		KVPair *Next = Current;
		while (Lvl >= 0)
		{
			Next = Current->Next[Lvl];
			while (Next && (Next->Key <= Key))
			{
				Current = Next;
				Next = Current->Next[Lvl];
			}
			Lvl--;
		}

		if (Current->Key == Key)
		{
			return Current->Value;
		}
		static V DefaultItem;
		return DefaultItem;
	}

private:
	struct KVPair
	{
		K Key;
		V Value;
		UINT64 Level;
		KVPair *Next[MaxLvl];
		KVPair *Prev[MaxLvl];
	};

	static_assert(sizeof(KVPair) > 32);

	KVPair *Head;
	INT16 Level;
	UINT64 Sz;

	UINT8 RandLevel()
	{
		UINT8 Level = 1;

		/* Rand() generates a 64-bit number.
		 * The probability of any one bit in particular being 0 or 1
		 * is 25% for either case. We only care about two bits,
		 * so let's use the bottom 2 bits for this.
		 */
		while ((pantheon::Rand() & 0x3) && Level < MaxLvl)
		{
			Level++;
		}
		return Level % MaxLvl;
	}

	void Setup()
	{
		if (this->Head == nullptr)
		{
			this->Head = new KVPair();
			this->Level = 0;
			this->Head->Key = K() - 1; /* HACK: Sets this at a huge value for integers only */
			this->Head->Value = V();
			for (UINT8 Lvl = 0; Lvl < MaxLvl; Lvl++)
			{
				this->Head->Next[Lvl] = this->Head;
				this->Head->Prev[Lvl] = this->Head;
			}		
		}
	}
};

}
#endif