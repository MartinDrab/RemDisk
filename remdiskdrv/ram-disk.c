
#include <ntifs.h>
#include <wdf.h>
#include "preprocessor.h"
#include "utils.h"
#include "remdisk-types.h"
#include "remdisk-device.h"
#include "ram-disk.h"


/************************************************************************/
/*               HELPER FUNCTIONS                                       */
/************************************************************************/

static VOID _RAMDiskWriteMBR(_In_ PREMDISK_DEVICE_EXTENSION Extension)
{
	LARGE_INTEGER systemTime;
	PMASTER_BOOT_RECORD mbr = (PMASTER_BOOT_RECORD)Extension->Parameters.RAMDisk.DiskImage;
	PMBR_PARTITION firstParttion = mbr->PrimaryPartitions;
	DEBUG_ENTER_FUNCTION("Extension=0x%p", Extension);

	RtlSecureZeroMemory(mbr, sizeof(MASTER_BOOT_RECORD));
	KeQuerySystemTime(&systemTime);
	mbr->DiskId = RtlRandomEx(&systemTime.LowPart);
	firstParttion->Flags = 0;
	firstParttion->StartCHS1 = 0xff;
	firstParttion->StartCHS2 = 0xff;
	firstParttion->StartCHS3 = 0xff;
	firstParttion->Type = 7;
	firstParttion->EndCHS1 = 0xff;
	firstParttion->EndCHS2 = 0xff;
	firstParttion->EndCHS3 = 0xff;
	firstParttion->LBAStart = 1;
	firstParttion->LBAEnd = (ULONG)(Extension->DiskSize / 512 - 1);
	mbr->Byte55h = 0x55;
	mbr->ByteAAh = 0xAA;
	if (ExtDiskEncrypted(Extension))
		XEXEncrypt(Extension->XEXKey, Extension->DiskGeometry.BytesPerSector, 0, Extension->DiskGeometry.BytesPerSector, (PUCHAR)mbr);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


static VOID _RAMDiskEvtIoRead(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t Length)
{
	WDFWORKITEM qc = NULL;
	PREMDISK_DEVICE_EXTENSION devExt = QueueGetExtension(Queue)->DeviceExtension;
	NTSTATUS status = STATUS_INVALID_PARAMETER;
	WDF_REQUEST_PARAMETERS Parameters;
	LARGE_INTEGER ByteOffset;
	WDFMEMORY hMemory;
	_Analysis_assume_(Length > 0);
	DEBUG_ENTER_FUNCTION("Queue=0x%p; Request=0x%p; Length=%Iu", Queue, Request, Length);

	status = UtilsQCCreate(Request, &qc);
	if (NT_SUCCESS(status)) {
		if (Length > 0) {
			WDF_REQUEST_PARAMETERS_INIT(&Parameters);
			WdfRequestGetParameters(Request, &Parameters);
			ByteOffset.QuadPart = Parameters.Parameters.Read.DeviceOffset;
			status = WdfRequestRetrieveOutputMemory(Request, &hMemory);
			if (NT_SUCCESS(status)) {
				RAMDiskAcquireRegionLocks(devExt, ByteOffset.QuadPart, Length);
				status = WdfMemoryCopyFromBuffer(hMemory, 0, devExt->Parameters.RAMDisk.DiskImage + ByteOffset.QuadPart, Length);
				if (NT_SUCCESS(status) && ExtDiskEncrypted(devExt)) {
					PUCHAR buffer = WdfMemoryGetBuffer(hMemory, NULL);

					XEXDecrypt(devExt->XEXKey, devExt->DiskGeometry.BytesPerSector, ByteOffset.QuadPart, Length, buffer);
				}

				RAMDiskReleaseRegionLocks(devExt, ByteOffset.QuadPart, Length);
			}
		} else status = STATUS_SUCCESS;

		if (!NT_SUCCESS(status))
			Length = 0;

		UtilsQcRun(qc, status, Length);
	} else WdfRequestComplete(Request, status);


	DEBUG_EXIT_FUNCTION("status=0x%x, Length=%u", status, Length);
	return;
}


static VOID _RAMDiskEvtIoWrite(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t Length)
{
	WDFWORKITEM qc = NULL;
	PREMDISK_DEVICE_EXTENSION devExt = QueueGetExtension(Queue)->DeviceExtension;
	NTSTATUS status = STATUS_INVALID_PARAMETER;
	WDF_REQUEST_PARAMETERS Parameters;
	LARGE_INTEGER ByteOffset;
	WDFMEMORY hMemory = NULL;
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
					RAMDiskAcquireRegionLocks(devExt, ByteOffset.QuadPart, Length);
					status = WdfMemoryCopyToBuffer(hMemory, 0, devExt->Parameters.RAMDisk.DiskImage + ByteOffset.QuadPart, Length);
					if (NT_SUCCESS(status) && ExtDiskEncrypted(devExt))
						XEXEncrypt(devExt->XEXKey, devExt->DiskGeometry.BytesPerSector, ByteOffset.QuadPart, Length, devExt->Parameters.RAMDisk.DiskImage + ByteOffset.QuadPart);

					RAMDiskReleaseRegionLocks(devExt, ByteOffset.QuadPart, Length);
				}
			} else status = STATUS_MEDIA_WRITE_PROTECTED;
		} else status = STATUS_SUCCESS;

		if (!NT_SUCCESS(status))
			Length = 0;

		UtilsQcRun(qc, status, Length);
	} else WdfRequestComplete(Request, status);

	DEBUG_EXIT_FUNCTION("void, status=0x%x, Length=%u", status, Length);
	return;
}


/************************************************************************/
/*               PUBLIC FUNCTIONS                                       */
/************************************************************************/



VOID RAMDiskAcquireRegionLocks(PREMDISK_DEVICE_EXTENSION Extension, ULONG64 StartingOffset, ULONG64 Length)
{
	const size_t count = sizeof(Extension->Parameters.RAMDisk.RegionLocks) / sizeof(Extension->Parameters.RAMDisk.RegionLocks[0]);
	const ULONG64 regionSize = UTILS_ALIGN_UP(Extension->DiskSize, Extension->MaxTransferLength) / count;
	const ULONG64 first = StartingOffset / regionSize;
	const ULONG64 last = (StartingOffset + Length - 1) / regionSize;
	DEBUG_ENTER_FUNCTION("Extension=0x%p; StartingOffset=%I64u; Length=%I64u", Extension, StartingOffset, Length);

	for (ULONG64 i = first; i <= last; ++i)
		WdfSpinLockAcquire(Extension->Parameters.RAMDisk.RegionLocks[i]);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


VOID RAMDiskReleaseRegionLocks(PREMDISK_DEVICE_EXTENSION Extension, ULONG64 StartingOffset, ULONG64 Length)
{
	const size_t count = sizeof(Extension->Parameters.RAMDisk.RegionLocks) / sizeof(Extension->Parameters.RAMDisk.RegionLocks[0]);
	const ULONG64 regionSize = UTILS_ALIGN_UP(Extension->DiskSize, Extension->MaxTransferLength) / count;
	const ULONG64 first = StartingOffset / regionSize;
	const ULONG64 last = (StartingOffset + Length - 1) / regionSize;
	DEBUG_ENTER_FUNCTION("Extension=0x%p; StartingOffset=%I64u; Length=%I64u", Extension, StartingOffset, Length);

	for (ULONG64 i = last + 1; i > first; --i)
		WdfSpinLockRelease(Extension->Parameters.RAMDisk.RegionLocks[i - 1]);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


NTSTATUS RAMDiskReadRawMemory(PREMDISK_DEVICE_EXTENSION Extension, LONG64 ByteOffset, WDFMEMORY Memory)
{
	size_t length = 0;
	PVOID buffer = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Extension=0x%p; ByteOffset=%I64i; Memory=0x%p", Extension, ByteOffset, Memory);

	buffer = WdfMemoryGetBuffer(Memory, &length);
	status = RAMDiskReadRawBuffer(Extension, ByteOffset, buffer, length);

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}

NTSTATUS RAMDiskReadRawBuffer(PREMDISK_DEVICE_EXTENSION Extension, LONG64 ByteOffset, PVOID Buffer, SIZE_T Length)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Extension=0x%p; ByteOffset=%I64i; Buffer=0x%p; Length=%Iu", Extension, ByteOffset, Buffer, Length);

	RtlCopyMemory(Buffer, Extension->Parameters.RAMDisk.DiskImage + ByteOffset, Length);
	status = STATUS_SUCCESS;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS RAMDiskWriteRawMemory(PREMDISK_DEVICE_EXTENSION Extension, LONG64 ByteOffset, WDFMEMORY Memory)
{
	size_t length = 0;
	PVOID buffer = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Extension=0x%p; ByteOffset=%I64i; Memory=0x%p", Extension, ByteOffset, Memory);

	buffer = WdfMemoryGetBuffer(Memory, &length);
	status = RAMDiskWriteRawBuffer(Extension, ByteOffset, buffer, length);

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS RAMDiskWriteRawBuffer(PREMDISK_DEVICE_EXTENSION Extension, LONG64 ByteOffset, PVOID Buffer, SIZE_T Length)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Extension=0x%p; ByteOffset=%I64i; Buffer=0x%p; Length=%Iu", Extension, ByteOffset, Buffer, Length);

	RtlCopyMemory(Extension->Parameters.RAMDisk.DiskImage + ByteOffset, Buffer, Length);
	status = STATUS_SUCCESS;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS RAMDiskInit(WDFDEVICE Device, PREMDISK_DEVICE_EXTENSION Extension, PREMBUS_DISK_INFORMATION DiskInfo, WDF_EXECUTION_LEVEL *QueueLevel)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Device=0x%p; Extension=0x%p; DiskInfo=0x%p; QueueLevel=0x%p", Device, Extension, DiskInfo, QueueLevel);

	Extension->Parameters.RAMDisk.DiskImage = MmAllocateNonCachedMemory((SIZE_T)Extension->DiskSize);
	status = (Extension->Parameters.RAMDisk.DiskImage != NULL) ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
	if (NT_SUCCESS(status)) {
		RtlSecureZeroMemory(Extension->Parameters.RAMDisk.DiskImage, (SIZE_T)Extension->DiskSize);
		if (DiskInfo->BackingFileObject != NULL) {
			HANDLE fileHandle = NULL;

			status = ObOpenObjectByPointer(DiskInfo->BackingFileObject, OBJ_KERNEL_HANDLE, NULL, FILE_READ_DATA | SYNCHRONIZE, *IoFileObjectType, KernelMode, &fileHandle);
			if (NT_SUCCESS(status)) {
				ULONG chunkdSize = Extension->MaxTransferLength;
				LARGE_INTEGER offset;
				LARGE_INTEGER remaining;

				remaining.QuadPart = Extension->DiskSize;
				offset.QuadPart = 0;
				while (NT_SUCCESS(status) && remaining.QuadPart > 0) {
					IO_STATUS_BLOCK iosb;
					
					if (remaining.QuadPart < chunkdSize)
						chunkdSize = remaining.LowPart;
				
					status = ZwReadFile(fileHandle, NULL, NULL, NULL, &iosb, Extension->Parameters.RAMDisk.DiskImage + offset.QuadPart, chunkdSize, &offset, NULL);
					if (NT_SUCCESS(status)) {
						offset.QuadPart += chunkdSize;
						remaining.QuadPart -= chunkdSize;
					}
				}

				ObCloseHandle(fileHandle, KernelMode);
			}
		} else _RAMDiskWriteMBR(Extension);
		
		if (NT_SUCCESS(status)) {
			for (size_t i = 0; i < sizeof(Extension->Parameters.RAMDisk.RegionLocks) / sizeof(Extension->Parameters.RAMDisk.RegionLocks[0]); ++i) {
				WDF_OBJECT_ATTRIBUTES spinLockAttributes;

				WDF_OBJECT_ATTRIBUTES_INIT(&spinLockAttributes);
				spinLockAttributes.ParentObject = Device;
				status = WdfSpinLockCreate(&spinLockAttributes, Extension->Parameters.RAMDisk.RegionLocks + i);
				if (!NT_SUCCESS(status))
					break;
			}

			if (NT_SUCCESS(status)) {
				PREMDISK_CALLBACKS callbacks = &Extension->DiskCallbacks;

				memset(callbacks, 0, sizeof(REMDISK_CALLBACKS));
				callbacks->EvtRead = _RAMDiskEvtIoRead;
				callbacks->EvtWrite = _RAMDiskEvtIoWrite;
				callbacks->Init = RAMDiskInit;
				callbacks->Finit = RAMDiskFinit;
				callbacks->ReadRawBuffer = RAMDiskReadRawBuffer;
				callbacks->ReadRawMemory = RAMDiskReadRawMemory;
				callbacks->WriteRawBuffer = RAMDiskWriteRawBuffer;
				callbacks->WriteRawMemory = RAMDiskWriteRawMemory;
				callbacks->RegionsAcquireExclusive = RAMDiskReleaseRegionLocks;
				callbacks->RegionsAcquireShared = RAMDiskReleaseRegionLocks;
				callbacks->RegionsRelease = RAMDiskReleaseRegionLocks;
				Extension->Flags |= REMDISK_FLAG_ALLOCATED;
				*QueueLevel = WdfExecutionLevelDispatch;
			}
		}

		if (!NT_SUCCESS(status)) {
			MmFreeNonCachedMemory(Extension->Parameters.RAMDisk.DiskImage, (SIZE_T)Extension->DiskSize);
			Extension->Parameters.RAMDisk.DiskImage = NULL;
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x, *QueueLevel=%u", status, *QueueLevel);
	return status;
}


VOID RAMDiskFinit(PREMDISK_DEVICE_EXTENSION Extension)
{
	DEBUG_ENTER_FUNCTION("Extenson=0x%p", Extension);

	if (FlagOn(REMDISK_FLAG_ALLOCATED, Extension->Flags)) {
		MmFreeNonCachedMemory(Extension->Parameters.RAMDisk.DiskImage, (SIZE_T)Extension->DiskSize);
		Extension->Parameters.RAMDisk.DiskImage = NULL;
		Extension->Flags &= (~REMDISK_FLAG_ALLOCATED);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
