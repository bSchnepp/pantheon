#include <kern_string.hpp>
#include <kern_datatypes.hpp>

/**
 * @file Common/kern_string.cpp
 * \~english @brief Definition for a safe wrapper around strings for use
 * in the kernel.
 * \~english @author Brian Schnepp
 */


/**
 * \~english @brief Initializes a new string.
 * \~english @author Brian Schnepp
 */
pantheon::String::String()
{
	this->ContentLen = 0;
	this->DataLen = 0;
	this->Str = "";	
}

/**
 * \~english @brief Initializes a new string, based on an existing string.
 * \~english @details A string is created based on a reference to a pointer
 * of characters, which must be null terminated. The contents of the
 * string cannot be modified, and it is accessed as read-only. The underlying
 * string is assumed to be UTF-8 content, which allows for strings in languages
 * other than the basic Latin alphabet to also be supported.
 * \~english @author Brian Schnepp
 */
pantheon::String::String(const char *Str)
{
	this->ContentLen = 0;
	this->Str = Str;

	UINT64 StrIndex = 0;
	for (char Cur = Str[0]; Cur != '\0'; Cur = Str[++StrIndex])
	{		
	}
	this->DataLen = StrIndex;

	UINT64 StrCIndex = 0;
	for (char Cur = Str[0]; StrCIndex < this->DataLen; Cur = Str[StrCIndex])
	{
		if ((Cur & 0x80) == 0)
		{
			StrCIndex += 1;
		}
		else if ((Cur & 0xE0) == 0xC0)
		{
			StrCIndex += 2;
		}
		else if ((Cur & 0xF0) == 0xE0)
		{
			StrCIndex += 3;
		}
		else if ((Cur & 0xF8) == 0xF0)
		{
			StrCIndex += 4;
		}
		this->ContentLen++;
	}
}

pantheon::String::~String()
{

}

/**
 * \~english @brief Indexes a byte from a string.
 * \~english @details Obtains the raw byte value at a particular location
 * of the string. The index must be between [0, DataLength()).
 * \~english @author Brian Schnepp
 * \~english @return A byte from the given string, interpretted as a char.
 * @see DataLength
 */
[[nodiscard]]
char pantheon::String::operator[](UINT64 Index) const
{
	if (Index < this->DataLen)
	{
		return this->Str[Index];
	}
	return '\0';
}

/**
 * \~english @brief Checks for equality with another string.
 * \~english @author Brian Schnepp
 * \~english @return True if equal in data and content, false otherwise.
 */
[[nodiscard]]
bool pantheon::String::operator==(const String &Other) const
{
	if (Other.DataLen != this->DataLen)
	{
		return FALSE;
	}

	for (UINT64 Index = 0; Index < this->DataLen; ++Index)
	{
		if (Other.Str[Index] != this->Str[Index])
		{
			return FALSE;
		}
	}
	return TRUE;
}

/**
 * \~english @brief Obtains the length, in letters, of the string
 * \~english @author Brian Schnepp
 * \~english @return The length of the underlying string, in numbers of letters.
 */
[[nodiscard]] UINT64 pantheon::String::CharLength() const
{
	return this->ContentLen;
}

/**
 * \~english @brief Obtains the length, in data, of the string
 * \~english @author Brian Schnepp
 * \~english @return The length of the underlying string, in numbers of bytes 
 * used.
 */
[[nodiscard]] UINT64 pantheon::String::DataLength() const
{
	return this->DataLen;
}


/**
 * \~english @brief Alias for DataLength()
 * \~english @author Brian Schnepp
 * @see DataLength
 */
[[nodiscard]]
UINT64 pantheon::String::Length() const
{
	return this->DataLength();
}