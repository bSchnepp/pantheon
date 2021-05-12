#include <kern.h>
#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#include <PhyProtocol/DeviceTree/DeviceTree.hpp>

#ifndef _KERN_DRIVERS_HPP_
#define _KERN_DRIVERS_HPP_

void InitDriver(CHAR *DriverName, UINT64 Address);
void DriverHandleDTB(CHAR *DriverName, DeviceTreeBlob *CurState);
void FiniDriver(CHAR *DriverName, UINT64 Address);

#endif