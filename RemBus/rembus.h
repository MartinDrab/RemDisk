
#ifndef _RAMDISK_H_
#define _RAMDISK_H_

#pragma warning(disable:4201)  // nameless struct/union warning

#include <ntifs.h>
#include <wdf.h>
#include "remdisk-types.h"
#include "rembus-types.h"



#pragma warning(default:4201)

#include <wdf.h>
// #define NTSTRSAFE_LIB
#include <ntstrsafe.h>
#include "remdisk-types.h"

#define NT_DEVICE_NAME                  L"\\Device\\RemdiskBus"
#define DOS_DEVICE_NAME                 L"\\DosDevices\\RemdiskBus"

#define DOS_DEVNAME_LENGTH              (sizeof(DOS_DEVICE_NAME)+sizeof(WCHAR)*10)


#define DEVICE_DESC							L"REMDisk Device"
// \0 in the end is for double termination - required for MULTI_SZ string
#define  FILEDISK_DEVICE_ID					L"RemBus\\FileDisk\0"
#define  RAMDISK_DEVICE_ID					L"RemBus\\RAMDisk\0"
#define  ENCRAMDISK_DEVICE_ID				L"RemBus\\EncryptedRAMDisk\0"
#define  ENCFILEDISK_DEVICE_ID				L"RemBus\\EncryptedFileDisk\0"

#define  REMDISK_DEVICE_ID					L"RemBus\\RemDisk\0"


typedef struct _PDO_IDENTIFICATION_DESCRIPTION {
	WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER Header; // should contain this header
	ULONG DiskNumber;
	WCHAR HardwareId[128];
} PDO_IDENTIFICATION_DESCRIPTION, *PPDO_IDENTIFICATION_DESCRIPTION;


typedef struct _REMDISK_BUS_DEVICE_CONTEXT{
	ULONG DiskCount;
	UNICODE_STRING SymbolicLink;
	ERESOURCE DiskListLock;
	PREMBUS_DISK_INFORMATION Disks[256];
} REMDISK_BUS_DEVICE_CONTEXT, *PREMDISK_BUS_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(REMDISK_BUS_DEVICE_CONTEXT, GetBusDeviceContext)


typedef struct _REMDISK_BUS_QUEUE_EXTENSION {
	PREMDISK_BUS_DEVICE_CONTEXT DeviceExtension;
} REMDISK_BUS_QUEUE_EXTENSION, *PREMDISK_BUS_QUEUE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(REMDISK_BUS_QUEUE_EXTENSION, BusQueueGetExtension)




DRIVER_INITIALIZE DriverEntry;

#if KMDF_VERSION_MINOR >= 7

EVT_WDF_DRIVER_DEVICE_ADD RamDiskEvtDeviceAdd;
EVT_WDF_DEVICE_CONTEXT_CLEANUP RemDiskEvtDeviceContextCleanup;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL RemDiskEvtIoDeviceControl;

#else

#endif


NTSTATUS RemBusEvtDeviceAdd(IN WDFDRIVER Driver, IN PWDFDEVICE_INIT DeviceInit);
VOID RemBusEvtDeviceContextCleanup(IN WDFOBJECT Device);
VOID RemBusEvtIoDeviceControl(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t OutputBufferLength, IN size_t InputBufferLength, IN ULONG IoControlCode);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS RemBusEnumerateChildren(_In_ WDFDEVICE Device);
NTSTATUS RemBusEvtDeviceListCreatePdo(WDFCHILDLIST DeviceList, PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription, PWDFDEVICE_INIT ChildInit);



#endif
