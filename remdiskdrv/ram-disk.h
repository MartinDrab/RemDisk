
#ifndef __REMDISK_RAM_DISK_H__
#define __REMDISK_RAM_DISK_H__


#include <ntifs.h>
#include <wdf.h>
#include "remdisk-types.h"
#include "remdisk-device.h"



VOID RAMDiskAcquireRegionLocks(PREMDISK_DEVICE_EXTENSION Extension, ULONG64 StartingOffset, ULONG64 Length);
VOID RAMDiskReleaseRegionLocks(PREMDISK_DEVICE_EXTENSION Extension, ULONG64 StartingOffset, ULONG64 Length);

NTSTATUS RAMDiskReadRawMemory(PREMDISK_DEVICE_EXTENSION Extension, LONG64 ByteOffset, WDFMEMORY Memory);
NTSTATUS RAMDiskReadRawBuffer(PREMDISK_DEVICE_EXTENSION Extension, LONG64 ByteOffset, PVOID Buffer, SIZE_T Length);
NTSTATUS RAMDiskWriteRawMemory(PREMDISK_DEVICE_EXTENSION Extension, LONG64 ByteOffset, WDFMEMORY Memory);
NTSTATUS RAMDiskWriteRawBuffer(PREMDISK_DEVICE_EXTENSION Extension, LONG64 ByteOffset, PVOID Buffer, SIZE_T Length);

NTSTATUS RAMDiskInit(WDFDEVICE Device, PREMDISK_DEVICE_EXTENSION Extension, PREMBUS_DISK_INFORMATION DiskInfo, WDF_EXECUTION_LEVEL *QueueLevel);
VOID RAMDiskFinit(PREMDISK_DEVICE_EXTENSION Extension);



#endif 
