.section .text

.extern main
.extern svc_ExitProcess

.global _start
_start:
	bl main 		// Run actual user code
	bl svc_ExitProcess	// Terminate the process if we ever come back
	b .			// Give up and loop forever if that didnt work
