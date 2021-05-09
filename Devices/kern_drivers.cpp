#include <kern.h>
#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#if defined(EnablePL011)
#include <PL011/PL011.hpp>
#endif

#include <Common/PhyProtocol/PCI/PCIe.hpp>

void InitDriver(CHAR *DriverName, UINT64 Address)
{
	if (StringCompare((void*)DriverName, (void*)"pl011", 5))
	{
		/* Currently do nothing! BoardInit should have handled the
		 * uart we're actually going to use...
		 */
	}
	else if (StringCompare((void*)DriverName, (void*)"pcie", 4))
	{
		
	}
	else if (StringCompare((void*)DriverName, (void*)"pl061", 5))
	{
		
	}
	else if (StringCompare((void*)DriverName, (void*)"pl031", 5))
	{
		
	}	
}