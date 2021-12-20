#include <kern_integers.hpp>
#include <kern_datatypes.hpp>

#ifndef _PCIE_HPP_
#define _PCIE_HPP_

namespace pantheon::pcie
{

typedef struct ECAMEntry
{
	UINT64 BaseAddress;
	UINT16 PCISegmentStart;
	UINT8 PCIBusStart;
	UINT8 PCIBusEnd;;
	UINT32 RESERVED;
}__attribute__((packed)) ECAMEntry;

typedef struct ECAMTable
{
	CHAR MCFG[4];
	UINT32 Length;
	UINT8 Revision;
	UINT8 Checksum;
	UINT8 OEMID[6];
	UINT64 OEMTableID;
	UINT32 OEMRevision;
	UINT32 CreatorID;
	UINT32 CreatorRevision;
	UINT64 RESERVED;
	ECAMEntry *Entries;
}__attribute__((packed)) ECAMTable;

void InitPCIe(void *Address);

}

#endif