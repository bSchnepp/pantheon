.globl write_ttbr0_el1
.globl write_ttbr1_el1

write_ttbr0_el1:
	msr ttbr0_el1, x0
	ret

write_ttbr1_el1:
	msr ttbr1_el1, x0
	ret
