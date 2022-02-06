#ifndef PANTHEON_H_
#define PANTHEON_H_

#include <kern_datatypes.hpp>

#ifdef __cplusplus
extern "C"
{
#endif

extern "C" void svc_ExitProcess();
extern "C" void svc_LogText(const CHAR *Content);
extern "C" void *svc_AllocateBuffer(UINT64 Sz);
extern "C" BOOL svc_CreateThread(void (*Entry)(void*), VOID *Reserved, void *StackTop, UINT8 Priority);

extern "C" void svc_CreateNamedEvent(const char *Name, UINT8 *Read, UINT8 *Write);
extern "C" void svc_SignalEvent(UINT8 Handle);
extern "C" void svc_ClearEvent(UINT8 Handle);
extern "C" void svc_ResetEvent(UINT8 Handle);
extern "C" BOOL svc_PollEvent(UINT8 Handle);
extern "C" VOID svc_Yield();

#ifdef __cplusplus
}
#endif

#endif