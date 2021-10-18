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
