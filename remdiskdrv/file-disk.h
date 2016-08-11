
#ifndef __REMDISK_FILEDISK_H__
#define __REMDISK_FILEDISK_H__


#include <ntifs.h>
#include <wdf.h>
#include "remdisk-types.h"
#include "remdisk-device.h"


VOID FileDiskAcquireRegionLocksShared(PREMDISK_DEVICE_EXTENSION Extension, ULONG64 StartingOffset, ULONG64 Length);
VOID FileDiskAcquireRegionLocksExclusive(PREMDISK_DEVICE_EXTENSION Extension, ULONG64 StartingOffset, ULONG64 Length);
VOID FileDiskReleaseRegionLocks(PREMDISK_DEVICE_EXTENSION Extension, ULONG64 StartingOffset, ULONG64 Length);

NTSTATUS FileDiskReadRawMemory(PREMDISK_DEVICE_EXTENSION Extension, LONG64 ByteOffset, WDFMEMORY Memory);
NTSTATUS FileDiskReadRawBuffer(PREMDISK_DEVICE_EXTENSION Extension, LONG64 ByteOffset, PVOID Buffer, SIZE_T Length);
NTSTATUS FileDiskWriteRawMemory(PREMDISK_DEVICE_EXTENSION Extension, LONG64 ByteOffset, WDFMEMORY Memory);
NTSTATUS FileDiskWriteRawBuffer(PREMDISK_DEVICE_EXTENSION Extension, LONG64 ByteOffset, PVOID Buffer, SIZE_T Length);

NTSTATUS FileDiskInit(WDFDEVICE Device, PREMDISK_DEVICE_EXTENSION Extension, PREMBUS_DISK_INFORMATION DiskInfo, WDF_EXECUTION_LEVEL *QueueLevel);
VOID FileDiskFinit(PREMDISK_DEVICE_EXTENSION Extension);




#endif
