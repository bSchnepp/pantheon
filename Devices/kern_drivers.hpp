#include <kern.h>
#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#include <PhyProtocol/DeviceTree/DeviceTree.hpp>

#ifndef _KERN_DRIVERS_HPP_
#define _KERN_DRIVERS_HPP_

void InitDriver(const CHAR *DriverName, UINT64 Address);
void DriverHandleDTB(const CHAR *DriverName, DeviceTreeBlob *CurState);
void FiniDriver(const CHAR *DriverName, UINT64 Address);

void PerCoreInit();

#endif