#include <stdint.h>

#ifndef _KERN_DATATYPES_H_
#define _KERN_DATATYPES_H_

typedef void VOID;

typedef char CHAR;
typedef signed char BYTE;

typedef uint8_t  UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef uint64_t UINT64;

typedef int8_t  INT8;
typedef int16_t INT16;
typedef int32_t INT32;
typedef int64_t INT64;

typedef uint32_t BOOL;

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef TRUE
#define TRUE (1)
#endif

#endif