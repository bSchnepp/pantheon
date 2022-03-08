#include "vmm.hpp"
#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#include <arch/aarch64/arch.hpp>
#include <Common/Structures/kern_slab.hpp>

VOID pantheon::vmm::InvalidateTLB()
{
	/* Smash the TLB: we don't want invalid entries left around. */
	asm volatile(
		"isb\n"
		"tlbi vmalle1\n"
		"dsb ish\n"
		"dsb sy\n"
		"isb\n" ::: "memory");
}

VOID pantheon::vmm::PrintPageTables(pantheon::vmm::PageTable *Table)
{
	for (const pantheon::vmm::PageTableEntry &L0Entry : Table->Entries)
	{
		SERIAL_LOG("0x%lx\n", L0Entry.GetRawAttributes());
		if (L0Entry.IsMapped())
		{
			pantheon::vmm::PageTable *L1 = reinterpret_cast<pantheon::vmm::PageTable*>(pantheon::vmm::PhysicalToVirtualAddress(L0Entry.GetPhysicalAddressArea()));
			for (const pantheon::vmm::PageTableEntry &L1Entry : L1->Entries)
			{
				SERIAL_LOG("\t0x%lx\n", L1Entry.GetRawAttributes());
				if (L1Entry.IsMapped())
				{
					pantheon::vmm::PageTable *L2 = reinterpret_cast<pantheon::vmm::PageTable*>(pantheon::vmm::PhysicalToVirtualAddress(L1Entry.GetPhysicalAddressArea()));
					for (const pantheon::vmm::PageTableEntry &L2Entry : L2->Entries)
					{
						SERIAL_LOG("\t\t0x%lx\n", L2Entry.GetRawAttributes());
						if (L2Entry.IsMapped())
						{
							pantheon::vmm::PageTable *L3 = reinterpret_cast<pantheon::vmm::PageTable*>(pantheon::vmm::PhysicalToVirtualAddress(L2Entry.GetPhysicalAddressArea()));
							for (const pantheon::vmm::PageTableEntry &L3Entry : L3->Entries)
							{
								SERIAL_LOG("\t\t\t0x%lx\n", L3Entry.GetRawAttributes());
							}
						}
					}
				}
			}
		}
	}
}

VOID pantheon::vmm::PrintPageTablesNoZeroes(pantheon::vmm::PageTable *Table)
{
	UINT64 L0Index = 0;
	for (const pantheon::vmm::PageTableEntry &L0Entry : Table->Entries)
	{
		if (L0Entry.GetRawAttributes() != 0)
		{
			SERIAL_LOG("0x%lx\n", L0Entry.GetRawAttributes());
		}
		if (L0Entry.IsMapped())
		{
			UINT64 L1Index = 0;
			pantheon::vmm::PageTable *L1 = reinterpret_cast<pantheon::vmm::PageTable*>(pantheon::vmm::PhysicalToVirtualAddress(L0Entry.GetPhysicalAddressArea()));
			for (const pantheon::vmm::PageTableEntry &L1Entry : L1->Entries)
			{
				if (L1Entry.GetRawAttributes() != 0x00)
				{
					SERIAL_LOG("\t0x%lx\n", L1Entry.GetRawAttributes());
				}

				if (L1Entry.IsMapped())
				{
					UINT64 L2Index = 0;
					pantheon::vmm::PageTable *L2 = reinterpret_cast<pantheon::vmm::PageTable*>(pantheon::vmm::PhysicalToVirtualAddress(L1Entry.GetPhysicalAddressArea()));
					for (const pantheon::vmm::PageTableEntry &L2Entry : L2->Entries)
					{
						if (L2Entry.GetRawAttributes() != 0x00)
						{
							SERIAL_LOG("\t\t0x%lx\n", L2Entry.GetRawAttributes());
						}
						if (L2Entry.IsMapped())
						{
							UINT64 L3Index = 0;
							pantheon::vmm::PageTable *L3 = reinterpret_cast<pantheon::vmm::PageTable*>(pantheon::vmm::PhysicalToVirtualAddress(L2Entry.GetPhysicalAddressArea()));
							for (const pantheon::vmm::PageTableEntry &L3Entry : L3->Entries)
							{
								if (L3Entry.GetRawAttributes() != 0x00)
								{
									UINT64 VirtAddress = 0x00;
									VirtAddress |= (L0Index << ((3 * 9) + 12));
									VirtAddress |= (L1Index << ((2 * 9) + 12));
									VirtAddress |= (L2Index << ((1 * 9) + 12));
									VirtAddress |= (L3Index << ((0 * 9) + 12));

									SERIAL_LOG("\t\t\t0x%lx\t[mapped at 0x%lx, index %ld %ld %ld %ld]\n", L3Entry.GetRawAttributes(), VirtAddress, L0Index, L1Index, L2Index, L3Index);
								}
								L3Index++;
							}
						}
						L2Index++;
					}
				}
				L1Index++;
			}
			L0Index++;
		}
	}
}