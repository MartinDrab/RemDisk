
#ifndef __REMBUS_CHILDLIST_H__
#define __REMBUS_CHILDLIST_H__

#include <ntifs.h>
#include "remdisk-types.h"
#include "rembus.h"




NTSTATUS RemBusChildFindByDiskNumber(PREMDISK_BUS_DEVICE_CONTEXT DeviceContext, ULONG DiskNumber, BOOLEAN LockHeld, PREMBUS_DISK_INFORMATION *DiskInfo);
NTSTATUS RemBusChildInsert(PREMDISK_BUS_DEVICE_CONTEXT DeviceContext, PREMBUS_DISK_INFORMATION DiskInfo);
VOID RemBusChildRemove(PREMDISK_BUS_DEVICE_CONTEXT DeviceContext, PREMBUS_DISK_INFORMATION DiskInfo);
VOID RemBusChildListLock(PREMDISK_BUS_DEVICE_CONTEXT DeviceContext, BOOLEAN Shared);
VOID RemBusChildListUnlock(PREMDISK_BUS_DEVICE_CONTEXT DeviceContext);






#endif
