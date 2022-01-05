#include <kern.h>
#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#if defined(EnablePL011)
#include <PL011/PL011.hpp>
#endif

#if defined(__aarch64__)
#include <arch/aarch64/gic.hpp>
#endif

#include <Proc/kern_cpu.hpp>

#include <System/PhyProtocol/PCI/PCIe.hpp>

/* HACK: This should be put in a better place.. */
extern VOID PerCoreBoardInit();

extern char interrupt_table[];

void PerCoreInit()
{
	pantheon::CPU::LIDT(interrupt_table);
	PerCoreBoardInit();
}