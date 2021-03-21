#include <kern_integers.hpp>
#include <DeviceTree/DeviceTree.hpp>

fdt_header *dtb_ptr = nullptr;

/* clang-format: off */
#ifdef __cplusplus
extern "C"
{
#endif

void kern_init(fdt_header *dtb)
{
	dtb_ptr = dtb;
	volatile bool CheckMe = CheckHeader(dtb_ptr);
	for (;;)
	{
		/* Ensure CheckMe isn't optimized out */
		volatile int k = ((uint64_t)(dtb)) + CheckMe;
	}
}

#ifdef __cplusplus
}
#endif
/* clang-format: on */
