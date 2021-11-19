#ifndef PANTHEON_H_
#define PANTHEON_H_

#include <kern_datatypes.hpp>

#ifdef __cplusplus
extern "C"
{
#endif

extern "C" void svc_LogText(const CHAR *Content);
extern "C" void *svc_AllocateBuffer(UINT64 Sz);
extern "C" BOOL svc_CreateThread(void (*Entry)(void*), VOID *Reserved, void *StackTop, UINT8 Priority);

#ifdef __cplusplus
}
#endif

#endif