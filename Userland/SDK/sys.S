.globl svc_ExitProcess
svc_ExitProcess:
	mov w8, #0
	svc #0
	ret

.globl svc_ForkProcess
svc_ForkProcess:
	mov w8, #1
	svc #0
	ret

.globl svc_LogText
svc_LogText:
	mov w8, #2
	svc #0
	ret

.globl svc_AllocateBuffer
svc_AllocateBuffer:
	mov w8, #3
	svc #0
	ret

.globl svc_CreateThread
svc_CreateThread:
	mov w8, #4
	svc #0
	ret

.globl svc_CreateNamedEvent
svc_CreateNamedEvent:
	mov w8, #5
	svc #0
	ret

.globl svc_SignalEvent
svc_SignalEvent:
	mov w8, #6
	svc #0
	ret

.globl svc_ClearEvent
svc_ClearEvent:
	mov w8, #7
	svc #0
	ret

.globl svc_ResetEvent
svc_ResetEvent:
	mov w8, #8
	svc #0
	ret

.globl svc_PollEvent
svc_PollEvent:
	mov w8, #9
	svc #0
	ret

.globl svc_Yield
svc_Yield:
	mov w8, #10
	svc #0
	ret

.globl svc_ExitThread
svc_ExitThread:
	mov w8, #11
	svc #0
	ret

.globl svc_Execute
svc_Execute:
	mov w8, #12
	svc #0
	ret

.globl svc_CreatePort
svc_CreatePort:
	mov w8, #13
	svc #0
	ret
