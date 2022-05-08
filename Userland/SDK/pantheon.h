#ifndef PANTHEON_H_
#define PANTHEON_H_

#include <kern_result.hpp>
#include <kern_datatypes.hpp>

extern "C" pantheon::Result svc_ExitProcess();
extern "C" pantheon::Result svc_LogText(const CHAR *Content);
extern "C" pantheon::Result *svc_AllocateBuffer(UINT64 Sz);
extern "C" BOOL svc_CreateThread(void (*Entry)(void*), VOID *Reserved, void *StackTop, UINT8 Priority);

extern "C" pantheon::Result svc_CreateNamedEvent(const char *Name, UINT8 *Read, UINT8 *Write);
extern "C" pantheon::Result svc_SignalEvent(UINT8 Handle);
extern "C" pantheon::Result svc_ClearEvent(UINT8 Handle);
extern "C" pantheon::Result svc_ResetEvent(UINT8 Handle);
extern "C" BOOL svc_PollEvent(UINT8 Handle);
extern "C" pantheon::Result svc_Yield();
extern "C" pantheon::Result svc_Execute(UINT8 Handle);
extern "C" pantheon::Result svc_CreatePort(const CHAR *Name, INT64 MaxConnections, INT32 *HandleServer, INT32 *HandleClient);
extern "C" pantheon::Result svc_ConnectToPort(INT32 *ClientPortHandle, INT32 *OutClientConnectionHandle);
extern "C" pantheon::Result svc_ConnectToNamedPort(const char *Name, INT32 *OutClientConnectionHandle);

#endif