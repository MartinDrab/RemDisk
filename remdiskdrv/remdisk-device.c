
#include <ntifs.h>
#include <ntstrsafe.h>
#include <initguid.h>
#include <ntdddisk.h>
#include <wdf.h>
#include <devpkey.h>
#include "preprocessor.h"
#include "wdf-symbolic-link.h"
#include "utils.h"
#include "rembus-types.h"
#include "kernel-ioctls.h"
#include "ram-disk.h"
#include "file-disk.h"
#include "remdisk-device.h"


/************************************************************************/
/*                         HELPER TYPEDEFS                              */
/************************************************************************/

typedef enum _EREMDiskSymbolicLinkType {
	rdstPartition0,
	rdstDRX,
	rdstPhysicalDrive,
} EREMDiskSymbolicLinkType, *PEREMDiskSymbolicLinkType;


/************************************************************************/
/*                       HEPER FUNCTION                                 */
/************************************************************************/


static NTSTATUS _CreateSymbolicLink(PUNICODE_STRING DeviceName, WDFDEVICE Device, EREMDiskSymbolicLinkType Type)
{
	WDFSYMBOLICLINK wdfSymbolicLink = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PREMDISK_DEVICE_EXTENSION Extension = DeviceGetExtension(Device);
	DECLARE_UNICODE_STRING_SIZE(uLinkName, 128);
	DEBUG_ENTER_FUNCTION("DeviceName=\"%wZ\"; Device=0x%p; Type=%u", DeviceName, Device, Type);

	switch (Type) {
		case rdstPartition0:
			status = RtlUnicodeStringPrintf(&uLinkName, L"\\Device\\Harddisk%u\\Partition0", Extension->DiskNumber);
			break;
		case rdstDRX:
			status = RtlUnicodeStringPrintf(&uLinkName, L"\\Device\\Harddisk%u\\DR%u", Extension->DiskNumber, Extension->DiskNumber);
			break;
		case rdstPhysicalDrive:
			status = RtlUnicodeStringPrintf(&uLinkName, L"\\DosDevices\\PhysicalDrive%u", Extension->DiskNumber);
			break;
		default:
			status = STATUS_NOT_SUPPORTED;
			break;
	}

	if (NT_SUCCESS(status))
		status = WdfSymbolicLinkCreate(Device, &uLinkName, DeviceName, RAMDISK_TAG, &wdfSymbolicLink);

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


static NTSTATUS _RemDiskExtensionToEntry(PREMDISK_INFORMATION_ENTRY Entry, const REMDISK_DEVICE_EXTENSION *Extension, SIZE_T BufferLength)
{
	ULONG requiredLength = 0;
	UNICODE_STRING uFileName;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Entry=0x%p; Extension=0x%p; BufferLength=%zu", Entry, Extension, BufferLength);
	DEBUG_IRQL_LESS_OR_EQUAL(APC_LEVEL);

	RtlSecureZeroMemory(&uFileName, sizeof(uFileName));
	if (Extension->Parameters.FileDisk.FileName != NULL)
		uFileName = Extension->Parameters.FileDisk.FileName->Name;
	
	requiredLength = sizeof(REMDISK_INFORMATION_ENTRY) + uFileName.Length;
	Entry->NextEntryOffset = requiredLength;
	if (requiredLength <= BufferLength) {
		Entry->DiskNumber = Extension->DiskNumber;
		Entry->DiskSize.QuadPart = Extension->DiskSize;
		Entry->ParentDiskNumber = 0;
		Entry->State = Extension->State;
		Entry->Type = Extension->Type;
		Entry->Flags = Extension->Flags;
		Entry->FileNameLength = uFileName.Length;
		Entry->FileNameOffset = sizeof(REMDISK_INFORMATION_ENTRY);
		memcpy((PUCHAR)Entry + Entry->FileNameOffset, uFileName.Buffer, uFileName.Length);
		status = STATUS_SUCCESS;
	} else status = STATUS_BUFFER_TOO_SMALL;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


static VOID _SuspendRequests(PREMDISK_DEVICE_EXTENSION Extension)
{
	DEBUG_ENTER_FUNCTION("Extension=0x%p", Extension);

	IoReleaseRemoveLockAndWait(&Extension->PendingRemoveLock, Extension);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


static VOID _ResumeRequests(PREMDISK_DEVICE_EXTENSION Extension)
{
	KIRQL irql;
	WDFREQUEST returnedRequest = NULL;
	WDFREQUEST foundRequest = NULL;
	DEBUG_ENTER_FUNCTION("Extension=0x%p", Extension);

	KeAcquireSpinLock(&Extension->PendingSpinLock, &irql);
	IoInitializeRemoveLock(&Extension->PendingRemoveLock, 0, 0, 0);
	while (NT_SUCCESS(WdfIoQueueFindRequest(Extension->PendingQueue, NULL, NULL, NULL, &foundRequest))) {
		if (NT_SUCCESS(WdfIoQueueRetrieveFoundRequest(Extension->PendingQueue, foundRequest, &returnedRequest)))
			WdfRequestForwardToIoQueue(returnedRequest, WdfDeviceGetDefaultQueue(WdfIoQueueGetDevice(Extension->PendingQueue)));

		WdfObjectDereference(foundRequest);
	}

	KeReleaseSpinLock(&Extension->PendingSpinLock, irql);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


static NTSTATUS _RemDiskStateChange(PREMDISK_DEVICE_EXTENSION Extension, EREmDiskInfoState NewState)
{
	LARGE_INTEGER timeout;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Extension=0x%p; NewState=%u", Extension, NewState);

	timeout.QuadPart = -100000000;
	switch (NewState) {
		case rdisDecrypting:
		case rdisEncrypting:
		case rdisPasswordChange:
		case rdisSaving:
		case rdisLoading:
			status = KeWaitForSingleObject(&Extension->StateEvent, Executive, KernelMode, FALSE, &timeout);
			if (status == STATUS_TIMEOUT)
				status = STATUS_IO_TIMEOUT;

			if (NT_SUCCESS(status))
				Extension->State = NewState;
			break;
		case rdisWorking:
			Extension->State = NewState;
			KeSetEvent(&Extension->StateEvent, IO_NO_INCREMENT, FALSE);
			status = STATUS_SUCCESS;
			break;
		default:
			ASSERT(FALSE);
			break;
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


static NTSTATUS _RemDiskXXXCrypt(PREMDISK_DEVICE_EXTENSION Extension, BOOLEAN Encrypt)
{
	ULONG64 offset = 0;
	ULONG64 remaining = 0;
	PUCHAR buffer = NULL;
	SIZE_T chunkSize = 0;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Extension=0x%p; Encrypt=%u", Extension, Encrypt);

	chunkSize = Extension->MaxTransferLength;
	buffer = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool, chunkSize, RAMDISK_TAG);
	if (buffer != NULL) {
		status = _RemDiskStateChange(Extension, (Encrypt) ? rdisEncrypting : rdisDecrypting);
		if (NT_SUCCESS(status)) {
			remaining = Extension->DiskSize;
			status = STATUS_SUCCESS;
			while (NT_SUCCESS(status) && remaining > 0) {
				if (remaining < chunkSize)
					chunkSize = (SIZE_T)remaining;

				status = Extension->DiskCallbacks.ReadRawBuffer(Extension, offset, buffer, chunkSize);
				if (NT_SUCCESS(status)) {
					if (Encrypt)
						XEXEncrypt(Extension->XEXKey, Extension->DiskGeometry.BytesPerSector, offset, chunkSize, buffer);
					else XEXDecrypt(Extension->XEXKey, Extension->DiskGeometry.BytesPerSector, offset, chunkSize, buffer);

					status = Extension->DiskCallbacks.WriteRawBuffer(Extension, offset, buffer, chunkSize);
					if (NT_SUCCESS(status)) {
						offset += chunkSize;
						remaining -= chunkSize;
					}
				}
			}

			_RemDiskStateChange(Extension, rdisWorking);
		}

		ExFreePoolWithTag(buffer, RAMDISK_TAG);
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


static NTSTATUS _RemDiskSetPassword(PREMDISK_DEVICE_EXTENSION Extension, ERemDiskPasswordChangeType ChangeType, UCHAR Password[128], UCHAR OldPassword[128])
{
	UCHAR newKey[16];
	UCHAR oldKey[16];
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Extension=0x%p; ChangeType=%u; Password=0x%p; OldPassword=0x%p", Extension, ChangeType, Password, OldPassword);

	status = STATUS_SUCCESS;
	switch (ChangeType) {
		case rdpcChange: {
			if (ExtDiskEncrypted(Extension)) {
				DeriveKey(Password, sizeof(Password), newKey);
				DeriveKey(OldPassword, sizeof(OldPassword), oldKey);
				status = (memcmp(Extension->XEXKey, oldKey, sizeof(oldKey)) == 0) ? STATUS_SUCCESS : STATUS_ACCESS_DENIED;
				if (NT_SUCCESS(status)) {
					ULONG64 offset = 0;
					ULONG64 remaining = 0;
					PUCHAR buffer = NULL;
					SIZE_T chunkSize = 0;

					_SuspendRequests(Extension);
					status = _RemDiskStateChange(Extension, rdisPasswordChange);
					if (NT_SUCCESS(status)) {
						chunkSize = Extension->MaxTransferLength;
						buffer = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool, chunkSize, RAMDISK_TAG);
						if (buffer != NULL) {
							remaining = Extension->DiskSize;
							status = STATUS_SUCCESS;
							while (NT_SUCCESS(status) && remaining > 0) {
								if (remaining < chunkSize)
									chunkSize = (SIZE_T)remaining;

								status = Extension->DiskCallbacks.ReadRawBuffer(Extension, offset, buffer, chunkSize);
								if (NT_SUCCESS(status)) {
									XEXDecrypt(Extension->XEXKey, Extension->DiskGeometry.BytesPerSector, offset, chunkSize, buffer);
									XEXEncrypt(newKey, Extension->DiskGeometry.BytesPerSector, offset, chunkSize, buffer);
									status = Extension->DiskCallbacks.WriteRawBuffer(Extension, offset, buffer, chunkSize);
									if (NT_SUCCESS(status)) {
										offset += chunkSize;
										remaining -= chunkSize;
									}
								}
							}

							ExFreePoolWithTag(buffer, RAMDISK_TAG);
						} else status = STATUS_INSUFFICIENT_RESOURCES;

						if (NT_SUCCESS(status))
							RtlCopyMemory(Extension->XEXKey, newKey, sizeof(Extension->XEXKey));

						_RemDiskStateChange(Extension, rdisWorking);
					}

					_ResumeRequests(Extension);
				}
			} else status = STATUS_INVALID_PARAMETER;	
		} break;
		case rdpcSet: {
			_SuspendRequests(Extension);
			if (!ExtDiskEncrypted(Extension)) {
				RtlCopyMemory(Extension->XEXKey, newKey, sizeof(Extension->XEXKey));
				if (NT_SUCCESS(status)) {
					status = _RemDiskXXXCrypt(Extension, TRUE);
					if (NT_SUCCESS(status))
						Extension->Flags |= REMDISK_FLAG_ENCRYPTED;

					if (!NT_SUCCESS(status))
						RtlSecureZeroMemory(Extension->XEXKey, sizeof(Extension->XEXKey));
				}
			} else status = STATUS_ALREADY_REGISTERED;

			_ResumeRequests(Extension);
		} break;
		case rdpcClear: {
			_SuspendRequests(Extension);
			if (ExtDiskEncrypted(Extension) && memcmp(newKey, Extension->XEXKey, sizeof(Extension->XEXKey)) == 0) {
				status = _RemDiskXXXCrypt(Extension, FALSE);
				if (NT_SUCCESS(status)) {
					Extension->Flags &= (~REMDISK_FLAG_ENCRYPTED);					
					RtlSecureZeroMemory(Extension->XEXKey, sizeof(Extension->XEXKey));
				}
			} else status = STATUS_INVALID_PARAMETER;

			_ResumeRequests(Extension);
		} break;
		default:
			status = STATUS_INVALID_PARAMETER;
			break;
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


static NTSTATUS _RemDiskSave(PREMDISK_DEVICE_EXTENSION Extension, HANDLE FileHandle)
{
	ULONG64 offset = 0;
	PVOID buffer = NULL;
	ULONG chunkSize = 0;
	ULONG64 remaining = 0;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Extension=0x%p; FileHandle=0x%p", Extension, FileHandle);

	status = STATUS_SUCCESS;
	remaining = Extension->DiskSize;
	chunkSize = Extension->MaxTransferLength;
	buffer = ExAllocatePoolWithTag(NonPagedPool, chunkSize, RAMDISK_TAG);
	if (buffer != NULL) {
		_SuspendRequests(Extension);
		status = _RemDiskStateChange(Extension, rdisSaving);
		if (NT_SUCCESS(status)) {
			while (NT_SUCCESS(status) && remaining > 0) {
				IO_STATUS_BLOCK iosb;

				if (remaining < chunkSize)
					chunkSize = (ULONG)remaining;

				status = Extension->DiskCallbacks.ReadRawBuffer(Extension, offset, buffer, chunkSize);
				if (NT_SUCCESS(status)) {
					status = ZwWriteFile(FileHandle, NULL, NULL, NULL, &iosb, buffer, chunkSize, NULL, NULL);
					if (NT_SUCCESS(status)) {
						remaining -= chunkSize;
						offset += chunkSize;
					}
				}
			}

			_RemDiskStateChange(Extension, rdisWorking);
		}

		_ResumeRequests(Extension);
		ExFreePoolWithTag(buffer, RAMDISK_TAG);
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


static VOID _RemDiskEvtIoRead(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t Length)
{
	KIRQL irql;
	NTSTATUS status = STATUS_INVALID_PARAMETER;
	PREMDISK_DEVICE_EXTENSION devExt = QueueGetExtension(Queue)->DeviceExtension;
	WDF_REQUEST_PARAMETERS parameters;
	DEBUG_ENTER_FUNCTION("Queue=0x%p; Request=0x%p; Length=%Iu", Queue, Request, Length);

	WDF_REQUEST_PARAMETERS_INIT(&parameters);
	WdfRequestGetParameters(Request, &parameters);
	if (RemDiskCheckParameters(devExt, parameters.Parameters.Read.DeviceOffset, &Length)) {
		KeAcquireSpinLock(&devExt->PendingSpinLock, &irql);
		status = IoAcquireRemoveLock(&devExt->PendingRemoveLock, Request);
		KeReleaseSpinLock(&devExt->PendingSpinLock, irql);
		if (NT_SUCCESS(status)) {
			devExt->DiskCallbacks.EvtRead(Queue, Request, Length);
			IoReleaseRemoveLock(&devExt->PendingRemoveLock, Request);
		} else WdfRequestForwardToIoQueue(Request, devExt->PendingQueue);
	} else WdfRequestComplete(Request, status);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


static VOID _RemDiskEvtIoWrite(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t Length)
{
	KIRQL irql;
	NTSTATUS status = STATUS_INVALID_PARAMETER;
	PREMDISK_DEVICE_EXTENSION devExt = QueueGetExtension(Queue)->DeviceExtension;
	WDF_REQUEST_PARAMETERS parameters;
	DEBUG_ENTER_FUNCTION("Queue=0x%p; Request=0x%p; Length=%Iu", Queue, Request, Length);

	WDF_REQUEST_PARAMETERS_INIT(&parameters);
	WdfRequestGetParameters(Request, &parameters);
	if (RemDiskCheckParameters(devExt, parameters.Parameters.Write.DeviceOffset, &Length)) {
		KeAcquireSpinLock(&devExt->PendingSpinLock, &irql);
		status = IoAcquireRemoveLock(&devExt->PendingRemoveLock, Request);
		KeReleaseSpinLock(&devExt->PendingSpinLock, irql);
		if (NT_SUCCESS(status)) {
			devExt->DiskCallbacks.EvtWrite(Queue, Request, Length);
			IoReleaseRemoveLock(&devExt->PendingRemoveLock, Request);
		} else WdfRequestForwardToIoQueue(Request, devExt->PendingQueue);
	} else WdfRequestComplete(Request, status);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


static NTSTATUS _RemDiskPreprocessIrpCallback(WDFDEVICE Device, PIRP Irp)
{
	PIO_STACK_LOCATION irpStack = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Device=0x%p; Irp=0x%p", Device, Irp);

	irpStack = IoGetCurrentIrpStackLocation(Irp);
	switch (irpStack->MajorFunction) {
		case IRP_MJ_FLUSH_BUFFERS:
			status = STATUS_SUCCESS;
			break;
		default:
			status = STATUS_INVALID_DEVICE_REQUEST;
			break;
	}

	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;

}

/************************************************************************/
/*                       PUBLIC FUNCTION                                */
/************************************************************************/

NTSTATUS RemDiskEvtDeviceQueryRemove(_In_ WDFDEVICE Device)
{
	PREMDISK_DEVICE_EXTENSION devExt = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Device=0x%p", Device);
	
	devExt = DeviceGetExtension(Device);
	status = (devExt->PreventMediaRemoval && devExt->State == rdisWorking) ? STATUS_UNSUCCESSFUL : STATUS_SUCCESS;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


static NTSTATUS _ObtainDiskInformation(PDEVICE_OBJECT Device, PREMBUS_DISK_INFORMATION *DiskInfo)
{
	PIRP irp = NULL;
	IO_STATUS_BLOCK iosb;
	KEVENT waitEvent;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Device=0x%p; DiskInfo=0x%p", Device, DiskInfo);

	irp = IoBuildDeviceIoControlRequest(IOCTL_REMBUS_PDO_GET_DISKINFO, Device, NULL, 0, DiskInfo, sizeof(PREMBUS_DISK_INFORMATION), TRUE, &waitEvent, &iosb);
	if (irp != NULL) {
		KeInitializeEvent(&waitEvent, NotificationEvent, FALSE);
		status = IoCallDriver(Device, irp);
		if (status == STATUS_PENDING) {
			(VOID)KeWaitForSingleObject(&waitEvent, Executive, KernelMode, FALSE, NULL);
			status = iosb.Status;
		}
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x", status, *DiskInfo);
	return status;
}


static VOID _ComputeDriverGeometry(PREMDISK_DEVICE_EXTENSION Extension, PREMBUS_DISK_INFORMATION DiskInfo)
{
	PDISK_GEOMETRY dg = &Extension->DiskGeometry;
	DEBUG_ENTER_FUNCTION("Extension=0x%p; DiskInfo=0x%p", Extension, DiskInfo);

	dg->BytesPerSector = 512;
	dg->MediaType = RemovableMedia;
	dg->TracksPerCylinder = 1;
	dg->SectorsPerTrack = 1;
	dg->Cylinders.QuadPart = Extension->DiskSize / dg->BytesPerSector;

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


static VOID RemDiskEvtDeviceSurpriseRemoval(WDFDEVICE Device)
{
	PREMDISK_DEVICE_EXTENSION devExt = NULL;
	DEBUG_ENTER_FUNCTION("Device=0x%p", Device);

	devExt = DeviceGetExtension(Device);
	if (devExt->DiskCallbacks.SurpriseRemoval != NULL)
		devExt->DiskCallbacks.SurpriseRemoval(Device, devExt);
	
	devExt->State = rdisSurpriseRemoved;

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

NTSTATUS RemDiskEtvDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit)
{
	WDFDEVICE hFDO = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	WDF_OBJECT_ATTRIBUTES deviceAttributes;
	WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;
	PREMDISK_DEVICE_EXTENSION pDeviceExtension = NULL;
	DECLARE_UNICODE_STRING_SIZE(uDeviceName, 48);
	PREMBUS_DISK_INFORMATION diskInfo = NULL;
	PDEVICE_OBJECT pdo = NULL;
	DEBUG_ENTER_FUNCTION("Driver=0x%p; DeviceInit=0x%p", Driver, DeviceInit);
	DEBUG_IRQL_LESS_OR_EQUAL(APC_LEVEL);

	pdo = WdfFdoInitWdmGetPhysicalDevice(DeviceInit);
	if (pdo == NULL) {
		status = STATUS_NOT_FOUND;
		goto Cleanup;
	}

	status = _ObtainDiskInformation(pdo, &diskInfo);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	DEBUG_PRINT_LOCATION("diskInfo: 0x%p", diskInfo);
	status = RtlUnicodeStringPrintf(&uDeviceName, L"\\Device\\RemDisk%u", diskInfo->DiskNumber);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	status = WdfDeviceInitAssignName(DeviceInit, &uDeviceName);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_DISK);
	WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoDirect);
	WdfDeviceInitSetExclusive(DeviceInit, FALSE);
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, REMDISK_DEVICE_EXTENSION);
	deviceAttributes.EvtCleanupCallback = RemDiskEvtDeviceContextCleanup;
	deviceAttributes.ExecutionLevel = WdfExecutionLevelPassive;
	status = WdfDeviceInitAssignSDDLString(DeviceInit, &SDDL_DEVOBJ_SYS_ALL_ADM_ALL);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	WDF_PNPPOWER_EVENT_CALLBACKS pnpCallbacks;
	WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpCallbacks);
	pnpCallbacks.EvtDeviceSurpriseRemoval = RemDiskEvtDeviceSurpriseRemoval;
	WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpCallbacks);

	status = WdfDeviceInitAssignWdmIrpPreprocessCallback(DeviceInit, _RemDiskPreprocessIrpCallback, IRP_MJ_FLUSH_BUFFERS, NULL, 0);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &hFDO);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IdleCannotWakeFromS0);
	idleSettings.IdleTimeout = 1000; // 1-sec
	status = WdfDeviceAssignS0IdleSettings(hFDO, &idleSettings);
	if (!NT_SUCCESS(status))
		return status;

	pDeviceExtension = DeviceGetExtension(hFDO);
	RtlSecureZeroMemory(pDeviceExtension, sizeof(REMDISK_DEVICE_EXTENSION));
	pDeviceExtension->PreventMediaRemoval = FALSE;
	pDeviceExtension->DiskNumber = diskInfo->DiskNumber;
	pDeviceExtension->DiskSize = diskInfo->DiskSize;
	pDeviceExtension->Type = diskInfo->Type;
	pDeviceExtension->MaxTransferLength = diskInfo->MaxTranfserLength;
	pDeviceExtension->Flags = diskInfo->Flags;
	_ComputeDriverGeometry(pDeviceExtension, diskInfo);
	WdfDeviceSetAlignmentRequirement(hFDO, pDeviceExtension->DiskGeometry.BytesPerSector);

	KeInitializeSpinLock(&pDeviceExtension->PendingSpinLock);
	IoInitializeRemoveLock(&pDeviceExtension->PendingRemoveLock, 0, 0, 0);
	status = IoAcquireRemoveLock(&pDeviceExtension->PendingRemoveLock, pDeviceExtension);
	if (!NT_SUCCESS(status))
		goto Cleanup;
	
	DECLARE_UNICODE_STRING_SIZE(uDOSSymbolicLink, 128);
	status = RtlUnicodeStringPrintf(&uDOSSymbolicLink, L"\\DosDevices\\RemDisk%u", diskInfo->DiskNumber);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	status = WdfDeviceCreateSymbolicLink(hFDO, &uDOSSymbolicLink);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	OBJECT_ATTRIBUTES oa;
	DECLARE_UNICODE_STRING_SIZE(uPartition0Directory, 32);

	status = RtlUnicodeStringPrintf(&uPartition0Directory, HARDDISK_FOMRAT_STRING, diskInfo->DiskNumber);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	InitializeObjectAttributes(&oa, &uPartition0Directory, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE | OBJ_PERMANENT, NULL, NULL);
	status = ZwCreateDirectoryObject(&pDeviceExtension->Partition0Directory, DIRECTORY_ALL_ACCESS, &oa);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	status = _CreateSymbolicLink(&uDeviceName, hFDO, rdstPartition0);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	status = _CreateSymbolicLink(&uDeviceName, hFDO, rdstDRX);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	status = _CreateSymbolicLink(&uDeviceName, hFDO, rdstPhysicalDrive);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	if (FlagOn(REMDISK_FLAG_ENCRYPTED, pDeviceExtension->Flags))
		DeriveKey(diskInfo->Password, sizeof(diskInfo->Password), pDeviceExtension->XEXKey);

	WDF_EXECUTION_LEVEL queueExecutionLevel = WdfExecutionLevelInvalid;
	switch (diskInfo->Type) {
		case rdtRAMDisk:
			status = RAMDiskInit(hFDO, pDeviceExtension, diskInfo, &queueExecutionLevel);
			break;
		case rdtFileDisk:
			status = FileDiskInit(hFDO, pDeviceExtension, diskInfo, &queueExecutionLevel);
			break;
		default:
			status = STATUS_NOT_SUPPORTED;
			break;
	}

	if (!NT_SUCCESS(status))
		goto Cleanup;

	KeInitializeEvent(&pDeviceExtension->StateEvent, SynchronizationEvent, TRUE);
	pDeviceExtension->State = rdisWorking;

	WDFQUEUE queue = NULL;
	WDF_IO_QUEUE_CONFIG ioQueueConfig;
	WDF_OBJECT_ATTRIBUTES queueAttributes;

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&queueAttributes, REMDISK_QUEUE_EXTENSION);
	queueAttributes.ExecutionLevel = queueExecutionLevel;
	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchParallel);
	ioQueueConfig.EvtIoRead = _RemDiskEvtIoRead;
	ioQueueConfig.EvtIoWrite = _RemDiskEvtIoWrite;
	ioQueueConfig.EvtIoDeviceControl = RemDiskEvtIoDeviceControl;
	ioQueueConfig.EvtIoInternalDeviceControl = RemDiskEvtIoInternalDeviceControl;
	__analysis_assume(ioQueueConfig.EvtIoStop != 0);
	status = WdfIoQueueCreate(hFDO, &ioQueueConfig, &queueAttributes, &queue);
	__analysis_assume(ioQueueConfig.EvtIoStop == 0);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	QueueGetExtension(queue)->DeviceExtension = pDeviceExtension;

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&queueAttributes, REMDISK_QUEUE_EXTENSION);
	queueAttributes.ExecutionLevel = queueExecutionLevel;
	WDF_IO_QUEUE_CONFIG_INIT(&ioQueueConfig, WdfIoQueueDispatchManual);
	__analysis_assume(ioQueueConfig.EvtIoStop != 0);
	status = WdfIoQueueCreate(hFDO, &ioQueueConfig, &queueAttributes, &pDeviceExtension->PendingQueue);
	__analysis_assume(ioQueueConfig.EvtIoStop == 0);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	QueueGetExtension(pDeviceExtension->PendingQueue)->DeviceExtension = pDeviceExtension;
	status = WdfDeviceCreateDeviceInterface(hFDO, &GUID_DEVINTERFACE_DISK, NULL);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	IoInitializeRemoveLock(&diskInfo->FDORemoveLock, RAMDISK_TAG, 0, 0);
	status = IoAcquireRemoveLock(&diskInfo->FDORemoveLock, pDeviceExtension);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	WdfObjectReference(hFDO);
	diskInfo->FDO = hFDO;
	InterlockedIncrement(&diskInfo->ReferenceCount);
	pDeviceExtension->DiskInfo = diskInfo;
Cleanup:
	if (diskInfo != NULL) {
		diskInfo->FDOAttachedStatus = status;
		KeSetEvent(&diskInfo->FDOAttachedEvent, IO_NO_INCREMENT, FALSE);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


VOID RemDiskEvtIoInternalDeviceControl(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t OutputBufferLength, IN size_t InputBufferLength, IN ULONG IoControlCode)
{
	size_t bufSize = 0;
	NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
	ULONG_PTR information = 0;
	PREMDISK_DEVICE_EXTENSION devExt = QueueGetExtension(Queue)->DeviceExtension;
	DEBUG_ENTER_FUNCTION("Queue=0x%p; Request=0x%p; OutputBufferLength=%Iu; InputBufferLength=%Iu; IoControlCode=0x%x", Queue, Request, OutputBufferLength, InputBufferLength, IoControlCode);

	switch (IoControlCode) {
		case IOCTL_REMDISK_INTERNAL_QUERY: {
			PREMDISK_INFORMATION_ENTRY output = NULL;

			information = sizeof(REMDISK_INFORMATION_ENTRY);
			status = WdfRequestRetrieveOutputBuffer(Request, information, &output, &bufSize);
			if (NT_SUCCESS(status)) {
				status = _RemDiskExtensionToEntry(output, devExt, bufSize);
				information = output->NextEntryOffset;
			}
		} break;
		case IOCTL_REMDISK_INTERNAL_PASSWORD: {
			PIOCTL_REMBUS_DISK_PASSWORD_CHANGE_INPUT input = NULL;

			status = WdfRequestRetrieveInputBuffer(Request, sizeof(IOCTL_REMBUS_DISK_PASSWORD_CHANGE_INPUT), &input, &bufSize);
			if (NT_SUCCESS(status))
				status = _RemDiskSetPassword(devExt, input->ChangeType, input->Password, input->OldPassword);
		} break;
		case IOCTL_REMDISK_INTERNAL_SAVE: {
			PHANDLE input = NULL;

			status = WdfRequestRetrieveInputBuffer(Request, sizeof(HANDLE), (PVOID *)&input, &bufSize);
			if (NT_SUCCESS(status))
				status = _RemDiskSave(devExt, *input);
		} break;
		case IOCTL_REMDISK_INTERNAL_WRITABLE: {
		} break;
		case IOCTL_DISK_INTERNAL_SET_VERIFY: {
			WDFDEVICE device = WdfIoQueueGetDevice(Queue);
			PDEVICE_OBJECT wdmDevice = WdfDeviceWdmGetDeviceObject(device);

			if (WdfRequestGetRequestorMode(Request) == KernelMode) {
				wdmDevice->Flags |= DO_VERIFY_VOLUME;
				status = STATUS_SUCCESS;
			}
		} break;
		case IOCTL_DISK_INTERNAL_CLEAR_VERIFY: {
			WDFDEVICE device = WdfIoQueueGetDevice(Queue);
			PDEVICE_OBJECT wdmDevice = WdfDeviceWdmGetDeviceObject(device);

			if (WdfRequestGetRequestorMode(Request) == KernelMode) {
				wdmDevice->Flags &= (~DO_VERIFY_VOLUME);
				status = STATUS_SUCCESS;
			}
		} break;
		default:
			status = STATUS_INVALID_DEVICE_REQUEST;
			break;
	}

	WdfRequestCompleteWithInformation(Request, status, information);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


VOID RemDiskEvtIoDeviceControl(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t OutputBufferLength, IN size_t InputBufferLength, IN ULONG IoControlCode)
{
	NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
	ULONG_PTR information = 0;
	size_t bufSize;
	PREMDISK_DEVICE_EXTENSION devExt = QueueGetExtension(Queue)->DeviceExtension;
	DEBUG_ENTER_FUNCTION("Queue=0x%p; Request=0x%p; OutputBufferLength=%Iu; InputBufferLength=%Iu; IoControlCode=0x%x", Queue, Request, OutputBufferLength, InputBufferLength, IoControlCode);

	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);

	switch (IoControlCode) {
	case IOCTL_STORAGE_GET_HOTPLUG_INFO: {
		PSTORAGE_HOTPLUG_INFO outputBuffer = NULL;

		information = sizeof(STORAGE_HOTPLUG_INFO);
		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(STORAGE_HOTPLUG_INFO), &outputBuffer, &bufSize);
		if (NT_SUCCESS(status)) {
			outputBuffer->Size = sizeof(STORAGE_HOTPLUG_INFO);
			outputBuffer->DeviceHotplug = FALSE;
			outputBuffer->MediaHotplug = FALSE;
			outputBuffer->MediaRemovable = FALSE;
			outputBuffer->WriteCacheEnableOverride = FALSE;
			status = STATUS_SUCCESS;
		}
	} break;
	case SMART_GET_VERSION: {
		PGETVERSIONINPARAMS outputBuffer = NULL;

		information = sizeof(GETVERSIONINPARAMS);
		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(GETVERSIONINPARAMS), &outputBuffer, &bufSize);
		if (NT_SUCCESS(status)) {
			RtlZeroMemory(outputBuffer, information);
		}
	} break;
	case IOCTL_STORAGE_MEDIA_REMOVAL:
	case IOCTL_DISK_MEDIA_REMOVAL: {
		PBOOLEAN inputBuffer = NULL;
		
		information = sizeof(BOOLEAN);
		status = WdfRequestRetrieveInputBuffer(Request, information, &inputBuffer, &bufSize);
		if (NT_SUCCESS(status))
			devExt->PreventMediaRemoval = *inputBuffer;
	} break;
	case IOCTL_STORAGE_GET_DEVICE_NUMBER: {
		PSTORAGE_DEVICE_NUMBER outputBuffer = NULL;

		information = sizeof(STORAGE_DEVICE_NUMBER);
		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(STORAGE_DEVICE_NUMBER), &outputBuffer, &bufSize);
		if (NT_SUCCESS(status)) {
			outputBuffer->DeviceType = FILE_DEVICE_DISK;
			outputBuffer->DeviceNumber = devExt->DiskNumber;
			outputBuffer->PartitionNumber = (ULONG)-1;
			status = STATUS_SUCCESS;
		}
	} break;
	case IOCTL_DISK_GET_LENGTH_INFO: {
		PGET_LENGTH_INFORMATION outputBuffer = NULL;

		information = sizeof(GET_LENGTH_INFORMATION);
		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(GET_LENGTH_INFORMATION), &outputBuffer, &bufSize);
		if (NT_SUCCESS(status)) {
			outputBuffer->Length.QuadPart = devExt->DiskSize;
			status = STATUS_SUCCESS;
		}
	} break;
	case IOCTL_STORAGE_READ_CAPACITY: {
		PSTORAGE_READ_CAPACITY outputBuffer = NULL;

		information = sizeof(STORAGE_READ_CAPACITY);
		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(STORAGE_READ_CAPACITY), &outputBuffer, &bufSize);
		if (NT_SUCCESS(status)) {
			outputBuffer->Size = sizeof(STORAGE_READ_CAPACITY);
			outputBuffer->Version = sizeof(STORAGE_READ_CAPACITY);
			outputBuffer->BlockLength = devExt->DiskGeometry.BytesPerSector;
			outputBuffer->DiskLength.QuadPart = devExt->DiskSize;
			outputBuffer->NumberOfBlocks.QuadPart = devExt->DiskSize / devExt->DiskGeometry.BytesPerSector;
		}
	} break;
	case IOCTL_DISK_UPDATE_PROPERTIES: {
		status = STATUS_SUCCESS;
	} break;
	case IOCTL_DISK_GET_DISK_ATTRIBUTES: {
		PGET_DISK_ATTRIBUTES outputBuffer = NULL;

		information = sizeof(GET_DISK_ATTRIBUTES);
		status = WdfRequestRetrieveOutputBuffer(Request, information, &outputBuffer, &bufSize);
		if (NT_SUCCESS(status)) {
			RtlSecureZeroMemory(outputBuffer, sizeof(GET_DISK_ATTRIBUTES));
			outputBuffer->Version = sizeof(GET_DISK_ATTRIBUTES);
			if (!FlagOn(REMDISK_FLAG_WRITABLE, devExt->Flags))
				outputBuffer->Attributes |= DISK_ATTRIBUTE_READ_ONLY;

			if (FlagOn(REMDISK_FLAG_OFFLINE, devExt->Flags))
				outputBuffer->Attributes |= DISK_ATTRIBUTE_OFFLINE;
		}
	} break;
	case IOCTL_DISK_SET_DISK_ATTRIBUTES: {
		PSET_DISK_ATTRIBUTES inputBuffer = NULL;

		status = WdfRequestRetrieveInputBuffer(Request, sizeof(SET_DISK_ATTRIBUTES), &inputBuffer, &bufSize);
		if (NT_SUCCESS(status)) {
			if (inputBuffer->Version == sizeof(SET_DISK_ATTRIBUTES)) {
				DWORDLONG mask = inputBuffer->AttributesMask;
				DWORDLONG values = inputBuffer->Attributes;

				if (FlagOn(DISK_ATTRIBUTE_OFFLINE, mask)) {
					if (FlagOn(DISK_ATTRIBUTE_OFFLINE, values))
						devExt->Flags |= REMDISK_FLAG_OFFLINE;
					else devExt->Flags &= (~REMDISK_FLAG_OFFLINE);
				}

				if (FlagOn(DISK_ATTRIBUTE_READ_ONLY, mask)) {
					if (!FlagOn(DISK_ATTRIBUTE_READ_ONLY, values))
						devExt->Flags |= REMDISK_FLAG_WRITABLE;
					else devExt->Flags &= (~REMDISK_FLAG_WRITABLE);
				}
			} else status = STATUS_INVALID_PARAMETER;
		}
	} break;
	case IOCTL_STORAGE_QUERY_PROPERTY: {
		PSTORAGE_PROPERTY_QUERY inputBuffer = NULL;

		status = WdfRequestRetrieveInputBuffer(Request, sizeof(STORAGE_PROPERTY_QUERY), &inputBuffer, &bufSize);
		if (NT_SUCCESS(status)) {
			status = STATUS_INVALID_DEVICE_REQUEST;
			switch (inputBuffer->PropertyId) {
			case StorageDeviceProperty: {
				if (inputBuffer->QueryType == PropertyStandardQuery) {
					PSTORAGE_DEVICE_DESCRIPTOR outputBuffer = NULL;

					information = sizeof(STORAGE_DESCRIPTOR_HEADER);
					status = WdfRequestRetrieveOutputBuffer(Request, sizeof(STORAGE_DESCRIPTOR_HEADER), &outputBuffer, &bufSize);
					if (NT_SUCCESS(status)) {
						outputBuffer->Size = sizeof(STORAGE_DEVICE_DESCRIPTOR);
						outputBuffer->Version = outputBuffer->Size;
						if (bufSize >= sizeof(STORAGE_DEVICE_DESCRIPTOR)) {
							memset(outputBuffer, 0, sizeof(STORAGE_DEVICE_DESCRIPTOR));
							outputBuffer->Size = sizeof(STORAGE_DEVICE_DESCRIPTOR);
							outputBuffer->Version = outputBuffer->Size;
							outputBuffer->DeviceType = 0x1F; // Simplified direct-access device
							outputBuffer->BusType = BusTypeVirtual;
							information = sizeof(STORAGE_DEVICE_DESCRIPTOR);
						}
					}
				} else if (inputBuffer->QueryType == PropertyExistsQuery) {
					status = STATUS_SUCCESS;
				}
			} break;
			case StorageAdapterProperty: {
				if (inputBuffer->QueryType == PropertyStandardQuery) {
					PSTORAGE_ADAPTER_DESCRIPTOR outputBuffer = NULL;

					information = sizeof(STORAGE_DESCRIPTOR_HEADER);
					status = WdfRequestRetrieveOutputBuffer(Request, sizeof(STORAGE_DESCRIPTOR_HEADER), &outputBuffer, &bufSize);
					if (NT_SUCCESS(status)) {
						outputBuffer->Size = sizeof(STORAGE_ADAPTER_DESCRIPTOR);
						outputBuffer->Version = outputBuffer->Size;
						if (bufSize >= sizeof(STORAGE_ADAPTER_DESCRIPTOR)) {
							memset(outputBuffer, 0, sizeof(STORAGE_ADAPTER_DESCRIPTOR));
							outputBuffer->Size = sizeof(STORAGE_ADAPTER_DESCRIPTOR);
							outputBuffer->Version = outputBuffer->Size;
							outputBuffer->MaximumTransferLength = devExt->MaxTransferLength;
							outputBuffer->AcceleratedTransfer = TRUE;
							outputBuffer->BusType = BusTypeVirtual;
							information = sizeof(STORAGE_ADAPTER_DESCRIPTOR);
						}
					}
				} else if (inputBuffer->QueryType == PropertyExistsQuery) {
					status = STATUS_SUCCESS;
				}
			} break;
			case StorageAccessAlignmentProperty: {
				if (inputBuffer->QueryType == PropertyStandardQuery) {
					PSTORAGE_ACCESS_ALIGNMENT_DESCRIPTOR outputBuffer = NULL;

					information = sizeof(STORAGE_DESCRIPTOR_HEADER);
					status = WdfRequestRetrieveOutputBuffer(Request, sizeof(STORAGE_DESCRIPTOR_HEADER), &outputBuffer, &bufSize);
					if (NT_SUCCESS(status)) {
						outputBuffer->Size = sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR);
						outputBuffer->Version = outputBuffer->Size;
						if (bufSize >= sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR)) {
							memset(outputBuffer, 0, sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR));
							outputBuffer->Size = sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR);
							outputBuffer->Version = outputBuffer->Size;
							outputBuffer->BytesPerLogicalSector = devExt->DiskGeometry.BytesPerSector;
							outputBuffer->BytesPerPhysicalSector = devExt->DiskGeometry.BytesPerSector;
							outputBuffer->BytesPerCacheLine = devExt->DiskGeometry.BytesPerSector;
							information = sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR);
						}
					}
				} else if (inputBuffer->QueryType == PropertyExistsQuery) {
					status = STATUS_SUCCESS;
				}
			} break;
			case StorageDeviceTrimProperty: {
				if (inputBuffer->QueryType == PropertyStandardQuery) {
					PDEVICE_TRIM_DESCRIPTOR outputBuffer = NULL;

					information = sizeof(STORAGE_DESCRIPTOR_HEADER);
					status = WdfRequestRetrieveOutputBuffer(Request, sizeof(STORAGE_DESCRIPTOR_HEADER), &outputBuffer, &bufSize);
					if (NT_SUCCESS(status)) {
						outputBuffer->Size = sizeof(DEVICE_TRIM_DESCRIPTOR);
						outputBuffer->Version = outputBuffer->Size;
						if (bufSize >= sizeof(DEVICE_TRIM_DESCRIPTOR)) {
							memset(outputBuffer, 0, sizeof(DEVICE_TRIM_DESCRIPTOR));
							outputBuffer->Size = sizeof(DEVICE_TRIM_DESCRIPTOR);
							outputBuffer->Version = outputBuffer->Size;
							outputBuffer->TrimEnabled = FALSE;
							information = sizeof(DEVICE_TRIM_DESCRIPTOR);
						}
					}
				} else if (inputBuffer->QueryType == PropertyExistsQuery) {
					status = STATUS_SUCCESS;
				}
			} break;
			}
		}
	} break;
	case IOCTL_DISK_GET_DRIVE_GEOMETRY_EX:  {
		PDISK_GEOMETRY_EX outputBuffer = NULL;

		information = sizeof(DISK_GEOMETRY_EX);
		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(DISK_GEOMETRY_EX), &outputBuffer, &bufSize);
		if (NT_SUCCESS(status)) {
			memset(outputBuffer, 0, bufSize);
			RtlCopyMemory(&outputBuffer->Geometry, &(devExt->DiskGeometry), sizeof(DISK_GEOMETRY));
			outputBuffer->DiskSize.QuadPart = devExt->DiskSize;
		}
	} break;
	case IOCTL_DISK_UPDATE_DRIVE_SIZE:
	case IOCTL_DISK_GET_DRIVE_GEOMETRY:  {
		PDISK_GEOMETRY outputBuffer;

		information = sizeof(DISK_GEOMETRY);
		status = WdfRequestRetrieveOutputBuffer(Request, sizeof(DISK_GEOMETRY), &outputBuffer, &bufSize);
		if (NT_SUCCESS(status)) {
			RtlCopyMemory(outputBuffer, &(devExt->DiskGeometry), sizeof(DISK_GEOMETRY));
			status = STATUS_SUCCESS;
		}
	} break;
	case IOCTL_DISK_CHECK_VERIFY:
	case IOCTL_STORAGE_CHECK_VERIFY:
	case IOCTL_STORAGE_CHECK_VERIFY2: {
		PULONG outputBuffer = NULL;

		if (OutputBufferLength >= sizeof(ULONG)) {
			status = WdfRequestRetrieveOutputBuffer(Request, sizeof(ULONG), &outputBuffer, &bufSize);
			if (NT_SUCCESS(status)) {
				*outputBuffer = 1;
				information = sizeof(ULONG);
				status = STATUS_SUCCESS;
			}
		} else status = STATUS_SUCCESS;
	} break;
	case IOCTL_DISK_IS_WRITABLE:
		status = (FlagOn(REMDISK_FLAG_WRITABLE, devExt->Flags)) ? STATUS_SUCCESS : STATUS_MEDIA_WRITE_PROTECTED;
		break;
	case IOCTL_DISK_COPY_DATA: {
		PDISK_COPY_DATA_PARAMETERS inputBuffer = NULL;

		status = WdfRequestRetrieveInputBuffer(Request, sizeof(DISK_COPY_DATA_PARAMETERS), &inputBuffer, &bufSize);
		if (NT_SUCCESS(status)) {
			if (FlagOn(REMDISK_FLAG_WRITABLE, devExt->Flags)) {
				if (inputBuffer->SourceOffset.QuadPart % devExt->DiskGeometry.BytesPerSector == 0 &&
					inputBuffer->DestinationOffset.QuadPart % devExt->DiskGeometry.BytesPerSector == 0 &&
					inputBuffer->CopyLength.QuadPart % devExt->DiskGeometry.BytesPerSector == 0) {
					PVOID buffer = NULL;

					buffer = ExAllocatePoolWithTag(NonPagedPool, devExt->MaxTransferLength, RAMDISK_TAG);
					if (buffer != NULL) {
						SIZE_T chunkSize = devExt->MaxTransferLength;
						ULONG64 remaining = inputBuffer->CopyLength.QuadPart;
						ULONG64 offset = 0;

						devExt->DiskCallbacks.RegionsAcquireExclusive(devExt, inputBuffer->DestinationOffset.QuadPart, inputBuffer->CopyLength.QuadPart);
						while (NT_SUCCESS(status) && remaining > 0) {
							if (remaining < chunkSize)
								chunkSize = (SIZE_T)remaining;

							devExt->DiskCallbacks.RegionsAcquireShared(devExt, inputBuffer->SourceOffset.QuadPart, chunkSize);
							status = devExt->DiskCallbacks.ReadRawBuffer(devExt, inputBuffer->SourceOffset.QuadPart + offset, buffer, chunkSize);
							devExt->DiskCallbacks.RegionsRelease(devExt, inputBuffer->SourceOffset.QuadPart, chunkSize);
							if (NT_SUCCESS(status)) {
								if (ExtDiskEncrypted(devExt)) {
									XEXDecrypt(devExt->XEXKey, devExt->DiskGeometry.BytesPerSector, inputBuffer->SourceOffset.QuadPart + offset, chunkSize, buffer);
									XEXEncrypt(devExt->XEXKey, devExt->DiskGeometry.BytesPerSector, inputBuffer->DestinationOffset.QuadPart + offset, chunkSize, buffer);
								}

								status = devExt->DiskCallbacks.WriteRawBuffer(devExt, inputBuffer->DestinationOffset.QuadPart + offset, buffer, chunkSize);
								if (NT_SUCCESS(status)) {
									remaining -= chunkSize;
									offset += chunkSize;
								}
							}
						}

						devExt->DiskCallbacks.RegionsRelease(devExt, inputBuffer->DestinationOffset.QuadPart, inputBuffer->CopyLength.QuadPart);
						ExFreePoolWithTag(buffer, RAMDISK_TAG);
					} else status = STATUS_INSUFFICIENT_RESOURCES;
				} else status = STATUS_INVALID_PARAMETER;
			} else status = STATUS_MEDIA_WRITE_PROTECTED;
		}
	} break;
	default:
		DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "IOCTL: 0x%x\n", IoControlCode);
		break;
	}

	WdfRequestCompleteWithInformation(Request, status, information);

	DEBUG_EXIT_FUNCTION("void, status=0x%x, information=%u", status, information);
	return;
}


VOID RemDiskEvtDeviceContextCleanup(IN WDFOBJECT Device)
{
	PREMDISK_DEVICE_EXTENSION pDeviceExtension = DeviceGetExtension(Device);
	DEBUG_ENTER_FUNCTION("Device=0x%p", Device);
	PAGED_CODE();
	
	if (pDeviceExtension->State == rdisWorking)
		pDeviceExtension->DiskCallbacks.Finit(pDeviceExtension);

	pDeviceExtension->State = rdisRemoved;
	if (pDeviceExtension->Partition0Directory != NULL) {
		ZwMakeTemporaryObject(pDeviceExtension->Partition0Directory);
		ZwClose(pDeviceExtension->Partition0Directory);
	}

	if (pDeviceExtension->DiskInfo != NULL) {
		PREMBUS_DISK_INFORMATION diskInfo = pDeviceExtension->DiskInfo;
		
		IoReleaseRemoveLockAndWait(&diskInfo->FDORemoveLock, pDeviceExtension);
		if (InterlockedDecrement(&diskInfo->ReferenceCount) == 0) {
			KeEnterCriticalRegion();
			ExAcquireResourceExclusiveLite(diskInfo->ListLock, TRUE);
			*diskInfo->ListPointer = NULL;
			ExReleaseResourceLite(diskInfo->ListLock);
			KeLeaveCriticalRegion();
			ExFreePoolWithTag(diskInfo, RAMDISK_TAG);
		}
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


BOOLEAN RemDiskCheckParameters(IN PREMDISK_DEVICE_EXTENSION DeviceExtension, IN LONGLONG ByteOffset, IN size_t *Length)
{
	BOOLEAN ret = FALSE;
	DEBUG_ENTER_FUNCTION("DeviceExtension=0x%p; ByteOffset=%I64u; Length=0x%p",  DeviceExtension, ByteOffset, Length);

	ret = (DeviceExtension->DiskSize >= *Length && ByteOffset >= 0 && (((*Length) & (DeviceExtension->DiskGeometry.BytesPerSector - 1)) == 0));
	if (ret) {
		if ((ULONGLONG)ByteOffset > DeviceExtension->DiskSize - *Length)
			*Length = (SIZE_T)(DeviceExtension->DiskSize - ByteOffset);
	}

	DEBUG_EXIT_FUNCTION("%u, *Length=%Iu", ret, *Length);
	return ret;
}
