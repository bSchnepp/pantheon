void *dtb_ptr = nullptr;

/* clang-format: off */
#ifdef __cplusplus
extern "C"
{
#endif

void kern_init(void *dtb)
{
	dtb_ptr = dtb;
	for (;;)
	{
	}
}

#ifdef __cplusplus
}
#endif
/* clang-format: on */
