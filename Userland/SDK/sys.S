.macro SVC_DEF name
	.section .text.\name
	.align 2
	.globl \name
	.type \name, @function
	.cfi_startproc
\name:
.endm

.macro SVC_END name
	.cfi_endproc
.endm

.section ".text"

SVC_DEF svc_ExitProcess
	mov w8, #0
	svc #0
	ret
SVC_END

SVC_DEF svc_ForkProcess
	mov w8, #1
	svc #0
	ret
SVC_END

SVC_DEF svc_LogText
	mov w8, #2
	svc #0
	ret
SVC_END

SVC_DEF svc_AllocateBuffer
	mov w8, #3
	svc #0
	ret
SVC_END

SVC_DEF svc_CreateThread
	mov w8, #4
	svc #0
	ret
SVC_END

SVC_DEF svc_CreateNamedEvent
	mov w8, #5
	svc #0
	ret
SVC_END

SVC_DEF svc_SignalEvent
	mov w8, #6
	svc #0
	ret
SVC_END

SVC_DEF svc_ClearEvent
	mov w8, #7
	svc #0
	ret
SVC_END

SVC_DEF svc_ResetEvent
	mov w8, #8
	svc #0
	ret
SVC_END

SVC_DEF svc_PollEvent
	mov w8, #9
	svc #0
	ret
SVC_END

SVC_DEF svc_Yield
	mov w8, #10
	svc #0
	ret
SVC_END

SVC_DEF svc_ExitThread
	mov w8, #11
	svc #0
	ret
SVC_END

SVC_DEF svc_Execute
	mov w8, #12
	svc #0
	ret
SVC_END

SVC_DEF svc_CreatePort
	mov w8, #13
	svc #0
	ret
SVC_END

SVC_DEF svc_ConnectToPort
	mov w8, #14
	svc #0
	ret
SVC_END

SVC_DEF svc_ConnectToNamedPort
	mov w8, #15
	svc #0
	ret
SVC_END

SVC_DEF svc_AcceptConnection
	mov w8, #16
	svc #0
	ret
SVC_END

SVC_DEF svc_ReplyAndRecieve
	mov w8, #17
	svc #0
	ret
SVC_END

