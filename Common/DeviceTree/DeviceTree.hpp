#include <kern_integers.hpp>

#ifndef _DEVICE_TREE_HPP_
#define _DEVICE_TREE_HPP_

typedef struct fdt_header
{
	BEIntegerU32 magic;
	BEIntegerU32 totalsize;
	BEIntegerU32 off_dt_struct;
	BEIntegerU32 off_dt_strings;
	BEIntegerU32 off_mem_rsvmap;
	BEIntegerU32 version;
	BEIntegerU32 last_comp_version;
	BEIntegerU32 boot_cpuid_phys;
	BEIntegerU32 size_dt_strings;
	BEIntegerU32 size_dt_struct;
}fdt_header;

typedef struct fdt_reserve_entry
{
	BEIntegerU64 address;
	BEIntegerU64 size;
}fdt_reserve_entry;

/* Note that these are host endianness. */
#define FDT_BEGIN_NODE (0x00000001)
#define FDT_END_NODE (0x00000002)
#define FDT_PROP (0x00000003)
#define FDT_NOP (0x00000004)
#define FDT_END (0x00000009)

typedef struct fdt_prop
{
	BEIntegerU32 len;
	BEIntegerU32 nameoff;
}fdt_prop;


bool CheckHeader(fdt_header *Header);

#endif