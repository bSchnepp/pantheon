#include <kern.h>
#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#include "DeviceTree/DeviceTree.hpp"

#ifndef _BOOT_DRIVER_HPP_
#define _BOOT_DRIVER_HPP_

void InitDriver(const CHAR *DriverName, UINT64 Address);
void DriverHandleDTB(const CHAR *DriverName, DeviceTreeBlob *CurState);
void FiniDriver(const CHAR *DriverName, UINT64 Address);

#endif