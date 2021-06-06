#include <kern_datatypes.hpp>

#ifndef _KERN_STRING_HPP_
#define _KERN_STRING_HPP_

namespace pantheon
{

class String
{

public:
	String();
	String(const char *Str);

	~String();

	[[nodiscard]] char operator[](UINT64 Index) const;
	[[nodiscard]] bool operator==(const String &Other) const;

	[[nodiscard]] UINT64 Length() const;

	[[nodiscard]] UINT64 CharLength() const;
	[[nodiscard]] UINT64 DataLength() const;

private:
	UINT64 ContentLen;
	UINT64 DataLen;

	const char *Str;
};


}


#endif