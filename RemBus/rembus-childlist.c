
#include <ntifs.h>
#include "preprocessor.h"
#include "remdisk-types.h"
#include "rembus-childlist.h"


/************************************************************************/
/*                       PUBLIC FUNCTIONS                               */
/************************************************************************/

VOID RemBusChildListLock(PREMDISK_BUS_DEVICE_CONTEXT DeviceContext, BOOLEAN Shared)
{
	KeEnterCriticalRegion();
	if (Shared)
		ExAcquireResourceSharedLite(&DeviceContext->DiskListLock, TRUE);
	else ExAcquireResourceExclusiveLite(&DeviceContext->DiskListLock, TRUE);

	return;
}


VOID RemBusChildListUnlock(PREMDISK_BUS_DEVICE_CONTEXT DeviceContext)
{
	ExReleaseResourceLite(&DeviceContext->DiskListLock);
	KeLeaveCriticalRegion();

	return;
}


NTSTATUS RemBusChildFindByDiskNumber(PREMDISK_BUS_DEVICE_CONTEXT DeviceContext, ULONG DiskNumber, BOOLEAN LockHeld, PREMBUS_DISK_INFORMATION *DiskInfo)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PREMBUS_DISK_INFORMATION di = NULL;
	DEBUG_ENTER_FUNCTION("DeviceContext=0x%p; DiskNumber=%u; LockHeld=%u; DiskInfo=0x%p", DeviceContext, DiskNumber, LockHeld, DiskInfo);

	status = STATUS_NOT_FOUND;
	if (!LockHeld)
		RemBusChildListLock(DeviceContext, TRUE);

	di = DeviceContext->Disks[DiskNumber];
	if (di != NULL) {
		*DiskInfo = di;
		status = STATUS_SUCCESS;
	}

	if (!LockHeld)
		RemBusChildListUnlock(DeviceContext);

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS RemBusChildInsert(PREMDISK_BUS_DEVICE_CONTEXT DeviceContext, PREMBUS_DISK_INFORMATION DiskInfo)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DeviceContext=0x%p; DiskInfo=0x%p", DeviceContext, DiskInfo);

	RemBusChildListLock(DeviceContext, FALSE);
	if (DeviceContext->Disks[DiskInfo->DiskNumber] == NULL) {
		DeviceContext->Disks[DiskInfo->DiskNumber] = DiskInfo;
		DiskInfo->ListLock = &DeviceContext->DiskListLock;
		DiskInfo->ListPointer = DeviceContext->Disks + DiskInfo->DiskNumber;
		status = STATUS_SUCCESS;
	} else status = STATUS_ALREADY_REGISTERED;

	RemBusChildListUnlock(DeviceContext);

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


VOID RemBusChildRemove(PREMDISK_BUS_DEVICE_CONTEXT DeviceContext, PREMBUS_DISK_INFORMATION DiskInfo)
{
	DEBUG_ENTER_FUNCTION("DeviceContext=0x%p; DiskInfo=0x%p", DeviceContext, DiskInfo);

	RemBusChildListLock(DeviceContext, FALSE);
	DeviceContext->Disks[DiskInfo->DiskNumber] = NULL;
	RemBusChildListUnlock(DeviceContext);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
