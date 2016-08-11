
#include <ntifs.h>
#include <wdf.h>
#include "preprocessor.h"
#include "wdf-rwlock.h"
#include "utils.h"
#include "remdisk-types.h"
#include "remdisk-device.h"
#include "file-disk.h"



/************************************************************************/
/*                 HELPER FUNCTIONS                                     */
/************************************************************************/

static NTSTATUS _ObtainFileName(PREMDISK_DEVICE_EXTENSION Extension, PFILE_OBJECT FileObject)
{
	ULONG len = 0;
	POBJECT_NAME_INFORMATION oni = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Extension=0x%p; FileObject=0x%p", Extension, FileObject);

	status = ObQueryNameString(FileObject, NULL, 0, &len);
	if (status == STATUS_INFO_LENGTH_MISMATCH) {
		len += sizeof(OBJECT_NAME_INFORMATION);
		oni = (POBJECT_NAME_INFORMATION)ExAllocatePoolWithTag(PagedPool, len, RAMDISK_TAG);
		if (oni != NULL) {
			status = ObQueryNameString(FileObject, oni, len, &len);
			if (NT_SUCCESS(status)) {
				Extension->Parameters.FileDisk.FileName = oni;
			}

			if (!NT_SUCCESS(status))
				ExFreePoolWithTag(oni, RAMDISK_TAG);
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


static NTSTATUS _FileDiskIOTargetCreate(WDFDEVICE Device, PREMBUS_DISK_INFORMATION DiskInfo)
{
	ACCESS_MASK desiredAccess = FILE_READ_DATA | SYNCHRONIZE;
	PREMDISK_DEVICE_EXTENSION devExt = DeviceGetExtension(Device);
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	WDF_OBJECT_ATTRIBUTES targetAttributes;
	DEBUG_ENTER_FUNCTION("Device=0x%p; DiskInfo=0x%p", Device, DiskInfo);

	status = _ObtainFileName(devExt, DiskInfo->BackingFileObject);
	if (NT_SUCCESS(status)) {
		if (FlagOn(REMDISK_FLAG_WRITABLE, DiskInfo->Flags))
			desiredAccess |= FILE_WRITE_DATA;

		status = ObOpenObjectByPointer(DiskInfo->BackingFileObject, OBJ_KERNEL_HANDLE, NULL, desiredAccess, *IoFileObjectType, KernelMode, &devExt->Parameters.FileDisk.FileHandle);
		if (NT_SUCCESS(status)) {
			WDF_OBJECT_ATTRIBUTES_INIT(&targetAttributes);
			targetAttributes.ParentObject = Device;
			status = WdfIoTargetCreate(Device, &targetAttributes, &devExt->Parameters.FileDisk.IOTarget);
			if (NT_SUCCESS(status)) {
				PDEVICE_OBJECT deviceObject = NULL;
				PFILE_OBJECT fileObject = NULL;
				WDF_IO_TARGET_OPEN_PARAMS openParams;

				ObReferenceObject(DiskInfo->BackingFileObject);
				fileObject = DiskInfo->BackingFileObject;
				deviceObject = IoGetRelatedDeviceObject(fileObject);
				WDF_IO_TARGET_OPEN_PARAMS_INIT_EXISTING_DEVICE(&openParams, deviceObject);
				openParams.TargetFileObject = fileObject;
				status = WdfIoTargetOpen(devExt->Parameters.FileDisk.IOTarget, &openParams);
				if (NT_SUCCESS(status)) {
					devExt->Parameters.FileDisk.BackingFileObject = fileObject;
					devExt->Parameters.FileDisk.BackingDeviceObject = deviceObject;
				}

				if (!NT_SUCCESS(status)) {
					ObDereferenceObject(fileObject);
					WdfObjectDelete(devExt->Parameters.FileDisk.IOTarget);
					devExt->Parameters.FileDisk.IOTarget = NULL;
				}
			}

			if (!NT_SUCCESS(status))
				ObCloseHandle(devExt->Parameters.FileDisk.FileHandle, KernelMode);
		}
	
		if (!NT_SUCCESS(status) && devExt->Parameters.FileDisk.FileName != NULL)
			ExFreePoolWithTag(devExt->Parameters.FileDisk.FileName, RAMDISK_TAG);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}

static VOID _FileDiskReadCompletion(_In_ WDFREQUEST Request, _In_ WDFIOTARGET Target, _In_ PWDF_REQUEST_COMPLETION_PARAMS Params, _In_ WDFCONTEXT Context)
{
	PREMDISK_REQUEST_CONTEXT rc = NULL;
	IO_STATUS_BLOCK iosb;
	DEBUG_ENTER_FUNCTION("Request=0x%p; Target=0x%p; Params=0x%p; Context=0x%p", Request, Target, Params, Context);

	rc = RemDiskRequestGetContext(Request);
	iosb = Params->IoStatus;
	if ((NT_SUCCESS(iosb.Status) || iosb.Information > 0) && rc->EncryptionKey != NULL)
		XEXDecrypt(rc->EncryptionKey, rc->BytesPerSector, rc->DeviceOffset.QuadPart, iosb.Information, rc->Buffer);

	FileDiskReleaseRegionLocks(rc->Extension, rc->DeviceOffset.QuadPart, rc->Length);
	WDF_REQUEST_REUSE_PARAMS reuseParams;
	WDF_REQUEST_REUSE_PARAMS_INIT(&reuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_SUCCESS);
	WdfRequestReuse(Request, &reuseParams);
	WdfObjectDelete(Request);
	UtilsQcRun(rc->QC, iosb.Status, iosb.Information);

	DEBUG_EXIT_FUNCTION("void, status = 0x%x, Information = %Iu", iosb.Status, iosb.Information);
	return;
}


static VOID _FileDiskWriteCompletion(_In_ WDFREQUEST Request, _In_ WDFIOTARGET Target, _In_ PWDF_REQUEST_COMPLETION_PARAMS Params, _In_ WDFCONTEXT Context)
{
	IO_STATUS_BLOCK iosb;
	PREMDISK_REQUEST_CONTEXT rc = NULL;
	DEBUG_ENTER_FUNCTION("Request=0x%p; Target=0x%p; Params=0x%p; Context=0x%p", Request, Target, Params, Context);

	UNREFERENCED_PARAMETER(Target);
	iosb = Params->IoStatus;
	rc = RemDiskRequestGetContext(Request);
	if (rc->SharedMemory) {
		WDF_REQUEST_REUSE_PARAMS reuseParams;

		WDF_REQUEST_REUSE_PARAMS_INIT(&reuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_SUCCESS);
		WdfRequestReuse(Request, &reuseParams);
	}

	FileDiskReleaseRegionLocks(rc->Extension, rc->DeviceOffset.QuadPart, rc->Length);

	WdfObjectDelete(Request);
	UtilsQcRun(rc->QC, iosb.Status, iosb.Information);

	DEBUG_EXIT_FUNCTION("void, status = 0x%x, Information = %Iu", iosb.Status, iosb.Information);
	return;
}


static VOID _FileDiskEvtIoRead(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t Length)
{
	WDFWORKITEM qc = NULL;
	WDFREQUEST lowerRequest = NULL;
	WDFMEMORY memory = NULL;
	PREMDISK_DEVICE_EXTENSION devExt = QueueGetExtension(Queue)->DeviceExtension;
	NTSTATUS status = STATUS_INVALID_PARAMETER;
	WDF_REQUEST_PARAMETERS Parameters;
	LARGE_INTEGER ByteOffset;
	_Analysis_assume_(Length > 0);
	DEBUG_ENTER_FUNCTION("Queue=0x%p; Request=0x%p; Length=%Iu", Queue, Request, Length);

	status = UtilsQCCreate(Request, &qc);
	if (NT_SUCCESS(status)) {
		if (Length > 0) {
			WDF_REQUEST_PARAMETERS_INIT(&Parameters);
			WdfRequestGetParameters(Request, &Parameters);
			ByteOffset.QuadPart = Parameters.Parameters.Read.DeviceOffset;
			status = WdfRequestRetrieveOutputMemory(Request, &memory);
			if (NT_SUCCESS(status)) {
				WDF_OBJECT_ATTRIBUTES requestAttributes;

				WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&requestAttributes, REMDISK_REQUEST_CONTEXT);
				requestAttributes.ParentObject = WdfIoQueueGetDevice(Queue);
				status = WdfRequestCreate(&requestAttributes, devExt->Parameters.FileDisk.IOTarget, &lowerRequest);
				if (NT_SUCCESS(status)) {
					status = WdfIoTargetFormatRequestForRead(devExt->Parameters.FileDisk.IOTarget, lowerRequest, memory, NULL, &ByteOffset.QuadPart);
					if (NT_SUCCESS(status)) {
						PREMDISK_REQUEST_CONTEXT rc = RemDiskRequestGetContext(lowerRequest);

						rc->EncryptionKey = NULL;
						rc->Extension = devExt;
						rc->Length = Length;
						rc->DeviceOffset = ByteOffset;
						rc->QC = qc;
						qc = NULL;
						if (ExtDiskEncrypted(devExt)) {
							rc->BytesPerSector = devExt->DiskGeometry.BytesPerSector;
							rc->EncryptionKey = devExt->XEXKey;
							rc->Buffer = WdfMemoryGetBuffer(memory, NULL);
						}

						FileDiskAcquireRegionLocksShared(devExt, ByteOffset.QuadPart, Length);
						WdfRequestSetCompletionRoutine(lowerRequest, _FileDiskReadCompletion, Request);
						if (!WdfRequestSend(lowerRequest, devExt->Parameters.FileDisk.IOTarget, WDF_NO_SEND_OPTIONS)) {
							status = WdfRequestGetStatus(lowerRequest);
							FileDiskReleaseRegionLocks(devExt, ByteOffset.QuadPart, Length);
							qc = rc->QC;
						}
					}

					if (!NT_SUCCESS(status))
						WdfObjectDelete(lowerRequest);
				}
			}
		} else status = STATUS_SUCCESS;
	
		if (!NT_SUCCESS(status))
			Length = 0;

		if (qc != NULL)
			UtilsQcRun(qc, status, Length);
	} else WdfRequestComplete(Request, status);

	DEBUG_EXIT_FUNCTION("status=0x%x, Length=%u", status, Length);
	return;
}


static VOID _FileDiskEvtIoWrite(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t Length)
{
	WDFWORKITEM qc = NULL;
	PREMDISK_DEVICE_EXTENSION devExt = QueueGetExtension(Queue)->DeviceExtension;
	WDFMEMORY newMemory = NULL;
	WDFMEMORY hMemory = NULL;
	NTSTATUS status = STATUS_INVALID_PARAMETER;
	WDF_REQUEST_PARAMETERS Parameters;
	LARGE_INTEGER ByteOffset;
	DEBUG_ENTER_FUNCTION("Queue=0x%p; Request=0x%p; Length=%Iu", Queue, Request, Length);

	status = UtilsQCCreate(Request, &qc);
	if (NT_SUCCESS(status)) {
		if (Length > 0) {
			_Analysis_assume_(Length > 0);
			WDF_REQUEST_PARAMETERS_INIT(&Parameters);
			WdfRequestGetParameters(Request, &Parameters);
			ByteOffset.QuadPart = Parameters.Parameters.Write.DeviceOffset;
			if (FlagOn(REMDISK_FLAG_WRITABLE, devExt->Flags)) {
				status = WdfRequestRetrieveInputMemory(Request, &hMemory);
				if (NT_SUCCESS(status)) {
					WDFREQUEST lowerRequest = NULL;

					if (ExtDiskEncrypted(devExt)) {
						PUCHAR buffer = WdfMemoryGetBuffer(hMemory, NULL);

						status = WdfMemoryCreate(WDF_NO_OBJECT_ATTRIBUTES, NonPagedPool, RAMDISK_TAG, Length, &newMemory, NULL);
						if (NT_SUCCESS(status)) {
							status = WdfMemoryCopyFromBuffer(newMemory, 0, buffer, Length);
							if (NT_SUCCESS(status)) {
								buffer = WdfMemoryGetBuffer(newMemory, NULL);
								XEXEncrypt(devExt->XEXKey, devExt->DiskGeometry.BytesPerSector, ByteOffset.QuadPart, Length, buffer);
							}
						}
					}

					WDF_OBJECT_ATTRIBUTES requestAttributes;
					
					WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&requestAttributes, REMDISK_REQUEST_CONTEXT);
					requestAttributes.ParentObject = WdfIoQueueGetDevice(Queue);
					status = WdfRequestCreate(&requestAttributes, devExt->Parameters.FileDisk.IOTarget, &lowerRequest);
					if (NT_SUCCESS(status)) {
						status = WdfIoTargetFormatRequestForWrite(devExt->Parameters.FileDisk.IOTarget, lowerRequest, ((newMemory != NULL) ? newMemory : hMemory), NULL, &ByteOffset.QuadPart);
						if (NT_SUCCESS(status)) {
							PREMDISK_REQUEST_CONTEXT rc = RemDiskRequestGetContext(lowerRequest);

							rc->SharedMemory = (newMemory == NULL);
							rc->Extension = devExt;
							rc->DeviceOffset = ByteOffset;
							rc->Length = Length;
							rc->QC = qc;
							qc = NULL;
							WdfRequestSetCompletionRoutine(lowerRequest, _FileDiskWriteCompletion, Request);
							FileDiskAcquireRegionLocksExclusive(devExt, ByteOffset.QuadPart, Length);
							if (!WdfRequestSend(lowerRequest, devExt->Parameters.FileDisk.IOTarget, WDF_NO_SEND_OPTIONS)) {
								FileDiskReleaseRegionLocks(devExt, ByteOffset.QuadPart, Length);
								status = WdfRequestGetStatus(lowerRequest);
								qc = rc->QC;
							}
						}

						if (!NT_SUCCESS(status))
							WdfObjectDelete(lowerRequest);
					}

					if (!NT_SUCCESS(status) && newMemory != NULL)
						WdfObjectDelete(newMemory);
				}
			} else status = STATUS_MEDIA_WRITE_PROTECTED;
		} else status = STATUS_SUCCESS;
	
		if (!NT_SUCCESS(status))
			Length = 0;

		if (qc != NULL)
			UtilsQcRun(qc, status, Length);
	} else WdfRequestComplete(Request, status);

	DEBUG_EXIT_FUNCTION("void, status=0x%x, Length=%u", status, Length);
	return;
}


/************************************************************************/
/*                PUBLIC FUNCTIONS                                      */
/************************************************************************/


VOID FileDiskAcquireRegionLocksShared(PREMDISK_DEVICE_EXTENSION Extension, ULONG64 StartingOffset, ULONG64 Length)
{
	const size_t count = sizeof(Extension->Parameters.FileDisk.RegionLocks) / sizeof(Extension->Parameters.FileDisk.RegionLocks[0]);
	const ULONG64 regionSize = UTILS_ALIGN_UP(Extension->DiskSize, Extension->MaxTransferLength) / count;
	const ULONG64 first = StartingOffset / regionSize;
	const ULONG64 last = (StartingOffset + Length - 1) / regionSize;
	DEBUG_ENTER_FUNCTION("Extension=0x%p; StartingOffset=%I64u; Length=%I64u", Extension, StartingOffset, Length);

	for (ULONG64 i = first; i <= last; ++i)
		WdfRWLockAcquireShared(Extension->Parameters.FileDisk.RegionLocks[i]);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


VOID FileDiskAcquireRegionLocksExclusive(PREMDISK_DEVICE_EXTENSION Extension, ULONG64 StartingOffset, ULONG64 Length)
{
	const size_t count = sizeof(Extension->Parameters.FileDisk.RegionLocks) / sizeof(Extension->Parameters.FileDisk.RegionLocks[0]);
	const ULONG64 regionSize = UTILS_ALIGN_UP(Extension->DiskSize, Extension->MaxTransferLength) / count;
	const ULONG64 first = StartingOffset / regionSize;
	const ULONG64 last = (StartingOffset + Length - 1) / regionSize;
	DEBUG_ENTER_FUNCTION("Extension=0x%p; StartingOffset=%I64u; Length=%I64u", Extension, StartingOffset, Length);

	for (ULONG64 i = first; i <= last; ++i)
		WdfRWLockAcquireExclusive(Extension->Parameters.FileDisk.RegionLocks[i]);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


VOID FileDiskReleaseRegionLocks(PREMDISK_DEVICE_EXTENSION Extension, ULONG64 StartingOffset, ULONG64 Length)
{
	const size_t count = sizeof(Extension->Parameters.FileDisk.RegionLocks) / sizeof(Extension->Parameters.FileDisk.RegionLocks[0]);
	const ULONG64 regionSize = UTILS_ALIGN_UP(Extension->DiskSize, Extension->MaxTransferLength) / count;
	const ULONG64 first = StartingOffset / regionSize;
	const ULONG64 last = (StartingOffset + Length - 1) / regionSize;
	DEBUG_ENTER_FUNCTION("Extension=0x%p; StartingOffset=%I64u; Length=%I64u", Extension, StartingOffset, Length);

	for (ULONG64 i = last + 1; i > first; --i)
		WdfRWLockRelease(Extension->Parameters.FileDisk.RegionLocks[i - 1]);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


NTSTATUS FileDiskReadRawMemory(PREMDISK_DEVICE_EXTENSION Extension, LONG64 ByteOffset, WDFMEMORY Memory)
{
	WDFREQUEST request = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Extension=0x%p; ByteOffset=%I64i; Memory=0x%p", Extension, ByteOffset, Memory);

	status = WdfRequestCreate(WDF_NO_OBJECT_ATTRIBUTES, Extension->Parameters.FileDisk.IOTarget, &request);
	if (NT_SUCCESS(status)) {
		status = WdfIoTargetFormatRequestForRead(Extension->Parameters.FileDisk.IOTarget, request, Memory, NULL, &ByteOffset);
		if (NT_SUCCESS(status)) {
			WDF_REQUEST_SEND_OPTIONS sendOptions;

			WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions, WDF_REQUEST_SEND_OPTION_SYNCHRONOUS);
			WdfRequestSend(request, Extension->Parameters.FileDisk.IOTarget, &sendOptions);
			status = WdfRequestGetStatus(request);
		}

		WdfObjectDelete(request);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS FileDiskReadRawBuffer(PREMDISK_DEVICE_EXTENSION Extension, LONG64 ByteOffset, PVOID Buffer, SIZE_T Length)
{
	WDFMEMORY memory = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Extension=0x%p; ByteOffset=%I64i; Buffer=0x%p; Length=%Iu", Extension, ByteOffset, Buffer, Length);

	status = WdfMemoryCreatePreallocated(WDF_NO_OBJECT_ATTRIBUTES, Buffer, Length, &memory);
	if (NT_SUCCESS(status)) {
		WdfObjectReference(memory);
		status = FileDiskReadRawMemory(Extension, ByteOffset, memory);
		WdfObjectDereference(memory);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS FileDiskWriteRawMemory(PREMDISK_DEVICE_EXTENSION Extension, LONG64 ByteOffset, WDFMEMORY Memory)
{
	WDFREQUEST request = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Extension=0x%p; ByteOffset=%I64i; Memory=0x%p", Extension, ByteOffset, Memory);

	status = WdfRequestCreate(WDF_NO_OBJECT_ATTRIBUTES, Extension->Parameters.FileDisk.IOTarget, &request);
	if (NT_SUCCESS(status)) {
		status = WdfIoTargetFormatRequestForWrite(Extension->Parameters.FileDisk.IOTarget, request, Memory, NULL, &ByteOffset);
		if (NT_SUCCESS(status)) {
			WDF_REQUEST_SEND_OPTIONS sendOptions;

			WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions, WDF_REQUEST_SEND_OPTION_SYNCHRONOUS);
			WdfRequestSend(request, Extension->Parameters.FileDisk.IOTarget, &sendOptions);
			status = WdfRequestGetStatus(request);
		}

		WdfObjectDelete(request);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS FileDiskWriteRawBuffer(PREMDISK_DEVICE_EXTENSION Extension, LONG64 ByteOffset, PVOID Buffer, SIZE_T Length)
{
	WDFMEMORY memory = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Extension=0x%p; ByteOffset=%I64i; Buffer=0x%p; Length=%Iu", Extension, ByteOffset, Buffer, Length);

	status = WdfMemoryCreatePreallocated(WDF_NO_OBJECT_ATTRIBUTES, Buffer, Length, &memory);
	if (NT_SUCCESS(status)) {
		WdfObjectReference(memory);
		status = FileDiskWriteRawMemory(Extension, ByteOffset, memory);
		WdfObjectDereference(memory);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS FileDiskInit(WDFDEVICE Device, PREMDISK_DEVICE_EXTENSION Extension, PREMBUS_DISK_INFORMATION DiskInfo, WDF_EXECUTION_LEVEL *QueueLevel)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Device=0x%p; Extension=0x%p; DiskInfo=0x%p; QueueLevel=0x%p", Device, Extension, DiskInfo, QueueLevel);

	status = _FileDiskIOTargetCreate(Device, DiskInfo);
	if (NT_SUCCESS(status)) {
		for (size_t i = 0; i < sizeof(Extension->Parameters.FileDisk.RegionLocks) / sizeof(Extension->Parameters.FileDisk.RegionLocks[0]); ++i) {
			status = WdfRWLockCreate(Device, Extension->Parameters.FileDisk.RegionLocks + i);
			if (!NT_SUCCESS(status))
				break;
		}

		if (NT_SUCCESS(status)) {
			PREMDISK_CALLBACKS callbacks = &Extension->DiskCallbacks;

			memset(callbacks, 0, sizeof(REMDISK_CALLBACKS));
			callbacks->EvtRead = _FileDiskEvtIoRead;
			callbacks->EvtWrite = _FileDiskEvtIoWrite;
			callbacks->Init = FileDiskInit;
			callbacks->Finit = FileDiskFinit;
			callbacks->ReadRawBuffer = FileDiskReadRawBuffer;
			callbacks->ReadRawMemory = FileDiskReadRawMemory;
			callbacks->WriteRawBuffer = FileDiskWriteRawBuffer;
			callbacks->WriteRawMemory = FileDiskWriteRawMemory;
			callbacks->RegionsAcquireExclusive = FileDiskReleaseRegionLocks;
			callbacks->RegionsAcquireShared = FileDiskReleaseRegionLocks;
			callbacks->RegionsRelease = FileDiskReleaseRegionLocks;
			*QueueLevel = WdfExecutionLevelPassive;
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x, *QueueLevel=%u", status, *QueueLevel);
	return status;
}


VOID FileDiskFinit(PREMDISK_DEVICE_EXTENSION Extension)
{
	DEBUG_ENTER_FUNCTION("Extenson=0x%p", Extension);

	if (Extension->Parameters.FileDisk.FileName != NULL)
		ExFreePoolWithTag(Extension->Parameters.FileDisk.FileName, RAMDISK_TAG);

	ObCloseHandle(Extension->Parameters.FileDisk.FileHandle, KernelMode);
	Extension->Parameters.FileDisk.FileHandle = NULL;
	WdfIoTargetClose(Extension->Parameters.FileDisk.IOTarget);
	WdfObjectDelete(Extension->Parameters.FileDisk.IOTarget);
	ObDereferenceObject(Extension->Parameters.FileDisk.BackingFileObject);
	Extension->Parameters.FileDisk.BackingFileObject = NULL;
	Extension->Parameters.FileDisk.BackingDeviceObject = NULL;

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
