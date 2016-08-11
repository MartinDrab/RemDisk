
#ifndef __REMDISK_TYPES_H__
#define __REMDISK_TYPES_H__

#include <ntifs.h>
#include <wdf.h>
#include <initguid.h>
#include "general-types.h"



#define RAMDISK_TAG                     'DmaR'  // "RamD"





typedef struct _REMBUS_DISK_INFORMATION {
	volatile LONG ReferenceCount;
	EREMDiskType Type;
	ULONG Flags;
	ULONG DiskNumber;
	ULONG64 DiskSize;
	KEVENT FDOAttachedEvent;
	NTSTATUS FDOAttachedStatus;
	PFILE_OBJECT BackingFileObject;
	ULONG MaxTranfserLength;
	UCHAR Password[128];
	BOOLEAN Inserted;
	WDFDEVICE FDO;
	IO_REMOVE_LOCK FDORemoveLock;
	PERESOURCE ListLock;
	struct _REMBUS_DISK_INFORMATION **ListPointer;
} REMBUS_DISK_INFORMATION, *PREMBUS_DISK_INFORMATION;


// {AF2A3B65-6D57-4AD0-B0E4-396426430B13}
DEFINE_GUID(GUID_DEVCLASS_REMDISK, 0xAF2A3B65, 0x6D57, 0x4AD0, 0xB0, 0xE4, 0x39, 0x64, 0x26, 0x43, 0x0B, 0x13);






#endif 
