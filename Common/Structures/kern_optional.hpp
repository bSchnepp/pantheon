#include <kern_datatypes.hpp>


#ifndef _KERN_OPTIONAL_HPP_
#define _KERN_OPTIONAL_HPP_

template <typename T>
class Optional
{
public:
	Optional()
	{
		this->Okay = FALSE;
		this->Value = T();
	}

	Optional(T v)
	{
		this->Okay = TRUE;
		this->Value = T(v);
	}

	~Optional()
	{

	}

	BOOL operator*()
	{
		return this->Okay;
	}

	T operator()()
	{
		if (this->Okay)
		{
			return this->Value;
		}
		return T();
	}

	BOOL GetOkay()
	{
		return this->Okay;
	}

	T &GetValue()
	{
		return this->Value;
	}

private:
	BOOL Okay;
	T Value;
};


#endif