#ifndef _KERN_MACRO_HPP_
#define _KERN_MACRO_HPP_

#define COMPILER_CONCAT_HELPER(x, y) x##y
#define COMPILER_CONCAT(x, y) COMPILER_CONCAT_HELPER(x, y)

#define COMPILER_ASSERT(condition) \
	typedef char COMPILER_CONCAT( \
		COMPILER_CONCAT( \
			COMPILER_CONCAT(assertion_failed_, __COUNTER__), _), \
		__LINE__)[(2 * (!!(condition))) - 1]

#endif