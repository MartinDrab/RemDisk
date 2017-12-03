
#ifndef _RAWPDO_H
#define _RAWPDO_H

#include <ntifs.h>
#include <ntdddisk.h>
#include <wdf.h>
#include "wdf-symbolic-link.h"
#include "wdf-rwlock.h"
#include "general-types.h"
#include "remdisk-types.h"
#include "utils.h"



#define HARDDISK_FOMRAT_STRING				L"\\Device\\Harddisk%u"
#define HARDDISK_STRING_LEN					(wcslen(HARDDISK_FOMRAT_STRING) + 3)


struct _REMDISK_DEVICE_EXTENSION;

typedef VOID(REMDISK_REGIONS_ACQUIRE_SHARED)(struct _REMDISK_DEVICE_EXTENSION *Extension, ULONG64 StartingOffset, ULONG64 Length);
typedef VOID(REMDISK_REGIONS_ACQUIRE_EXCLUSIVE)(struct _REMDISK_DEVICE_EXTENSION*Extension, ULONG64 StartingOffset, ULONG64 Length);
typedef VOID(REMDISK_REGONS_RELEASE)(struct _REMDISK_DEVICE_EXTENSION *Extension, ULONG64 StartingOffset, ULONG64 Length);
typedef NTSTATUS(REDISK_READ_RAW_MEMORY)(struct _REMDISK_DEVICE_EXTENSION *Extension, LONG64 ByteOffset, WDFMEMORY Memory);
typedef NTSTATUS(REMDISK_READ_RAW_BUFFER)(struct _REMDISK_DEVICE_EXTENSION *Extension, LONG64 ByteOffset, PVOID Buffer, SIZE_T Length);
typedef NTSTATUS(REMDISK_WRITE_RAW_MEMORY)(struct _REMDISK_DEVICE_EXTENSION *Extension, LONG64 ByteOffset, WDFMEMORY Memory);
typedef NTSTATUS(REMDISK_WRITE_RAW_BUFFER)(struct _REMDISK_DEVICE_EXTENSION *Extension, LONG64 ByteOffset, PVOID Buffer, SIZE_T Length);
typedef NTSTATUS(REMDISK_INIT)(WDFDEVICE Device, struct _REMDISK_DEVICE_EXTENSION *Extension, PREMBUS_DISK_INFORMATION DiskInfo, WDF_EXECUTION_LEVEL *QueueLevel);
typedef VOID(REMDISK_FINIT)(struct _REMDISK_DEVICE_EXTENSION *Extension);
typedef VOID(REMDISK_EVT_READ)(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t Length);
typedef VOID(REMDISK_EVT_WRITE)(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t Length);
typedef VOID(REMDISK_EVT_SURPRISE_REMOVAL)(_In_ WDFDEVICE Device, _In_ struct _REMDISK_DEVICE_EXTENSION *Extension);


typedef struct _REMDISK_CALLBACKS {
	REMDISK_REGIONS_ACQUIRE_SHARED *RegionsAcquireShared;
	REMDISK_REGIONS_ACQUIRE_EXCLUSIVE *RegionsAcquireExclusive;
	REMDISK_REGONS_RELEASE *RegionsRelease;
	REDISK_READ_RAW_MEMORY *ReadRawMemory;
	REMDISK_READ_RAW_BUFFER *ReadRawBuffer;
	REMDISK_WRITE_RAW_MEMORY *WriteRawMemory;
	REMDISK_WRITE_RAW_BUFFER *WriteRawBuffer;
	REMDISK_INIT *Init;
	REMDISK_FINIT *Finit;
	REMDISK_EVT_READ *EvtRead;
	REMDISK_EVT_WRITE *EvtWrite;
	REMDISK_EVT_SURPRISE_REMOVAL *SurpriseRemoval;
} REMDISK_CALLBACKS, *PREMDISK_CALLBACKS;



typedef struct _REMDISK_DEVICE_EXTENSION {
	ULONG DiskNumber;
	ULONG Flags;
	ULONG MaxTransferLength;
	EREMDiskType Type;
	ULONG64 DiskSize;
	DISK_GEOMETRY DiskGeometry;
	HANDLE Partition0Directory;
	UCHAR XEXKey[16];
	BOOLEAN PreventMediaRemoval;
	EREmDiskInfoState State;
	KEVENT StateEvent;
	WDFQUEUE PendingQueue;
	IO_REMOVE_LOCK PendingRemoveLock;
	PREMBUS_DISK_INFORMATION DiskInfo;
	KSPIN_LOCK PendingSpinLock;
	REMDISK_CALLBACKS DiskCallbacks;
	union {
		struct {
			WDFSPINLOCK RegionLocks[1024];
			PUCHAR DiskImage;
			UNICODE_STRING RemovalFileObjectName;
		} RAMDisk;
		struct {
			WDFRWLOCK RegionLocks[1024];
			WDFIOTARGET IOTarget;
			PFILE_OBJECT BackingFileObject;
			PDEVICE_OBJECT BackingDeviceObject;
			HANDLE FileHandle;
			POBJECT_NAME_INFORMATION FileName;
		} FileDisk;
	} Parameters;
} REMDISK_DEVICE_EXTENSION, *PREMDISK_DEVICE_EXTENSION;

#define ExtDiskEncrypted(aExtension)	(FlagOn(REMDISK_FLAG_ENCRYPTED, (aExtension)->Flags))

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(REMDISK_DEVICE_EXTENSION, DeviceGetExtension)

typedef struct _REMDISK_QUEUE_EXTENSION {
	PREMDISK_DEVICE_EXTENSION DeviceExtension;
} REMDISK_QUEUE_EXTENSION, *PREMDISK_QUEUE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(REMDISK_QUEUE_EXTENSION, QueueGetExtension)

typedef struct _REMDISK_REQUEST_CONTEXT {
	PUCHAR EncryptionKey;
	ULONG BytesPerSector;
	LARGE_INTEGER DeviceOffset;
	PVOID Buffer;
	SIZE_T Length;
	BOOLEAN SharedMemory;
	PREMDISK_DEVICE_EXTENSION Extension;
	WDFWORKITEM QC;
} REMDISK_REQUEST_CONTEXT, *PREMDISK_REQUEST_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(REMDISK_REQUEST_CONTEXT, RemDiskRequestGetContext)


VOID RemDiskEvtDeviceContextCleanup(IN WDFOBJECT Device);
VOID RemDiskEvtIoDeviceControl(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t OutputBufferLength, IN size_t InputBufferLength, IN ULONG IoControlCode);
VOID RemDiskEvtIoInternalDeviceControl(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t OutputBufferLength, IN size_t InputBufferLength, IN ULONG IoControlCode);

NTSTATUS RemDiskEtvDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit);

BOOLEAN RemDiskCheckParameters(IN PREMDISK_DEVICE_EXTENSION DeviceExtension, IN LONGLONG ByteOffset, IN size_t *Length);



#endif
