
#include <ntifs.h>
#include <wdf.h>
#include <initguid.h>
#include "preprocessor.h"
#include "remdisk-types.h"
#include "ioctls.h"
#include "utils.h"
#include "kernel-ioctls.h"
#include "rembus-childlist.h"
#include "rembus.h"



#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, RemBusEvtDeviceAdd)
#pragma alloc_text(PAGE, RemBusEvtDeviceContextCleanup)
#pragma alloc_text(PAGE, RemBusEvtIoDeviceControl)
#pragma alloc_text(PAGE, RemBusEvtDeviceListCreatePdo)
#endif

/************************************************************************/
/*                         HELPER FUNCTIONS                             */
/************************************************************************/

typedef struct _IIOCTL_COMPLETION_CONTEXT{
	WDFREQUEST UpperRequest;
	ULONG IoControlCode;
	union {
		struct {
			HANDLE FileHandle;
		} FileLoad;
		struct {
			HANDLE FileHandle;
		} FileSave;
	} Data;
} IIOCTL_COMPLETION_CONTEXT, *PIIOCTL_COMPLETION_CONTEXT;


static NTSTATUS _IIOCTLContextAlloc(ULONG IoControlCode, WDFREQUEST UpperRequest, PIIOCTL_COMPLETION_CONTEXT *Context)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PIIOCTL_COMPLETION_CONTEXT tmpContext = NULL;
	DEBUG_ENTER_FUNCTION("IoControlCode=0x%x; UpperRequest=0x%p; Context=0x%p", IoControlCode, Context);

	tmpContext = (PIIOCTL_COMPLETION_CONTEXT)ExAllocatePoolWithTag(NonPagedPool, sizeof(IIOCTL_COMPLETION_CONTEXT), RAMDISK_TAG);
	if (tmpContext != NULL) {
		tmpContext->IoControlCode = IoControlCode;
		tmpContext->UpperRequest = UpperRequest;
		*Context = tmpContext;
		status = STATUS_SUCCESS;
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x, Context=0x%p", status, *Context);
	return status;
}


static VOID _IIOCTLContextFree(PIIOCTL_COMPLETION_CONTEXT Context)
{
	DEBUG_ENTER_FUNCTION("Context=0x%p", Context);

	switch (Context->IoControlCode) {
		case IOCTL_REMDISK_INTERNAL_LOAD:
			ObCloseHandle(Context->Data.FileLoad.FileHandle, KernelMode);
			break;
		case IOCTL_REMDISK_INTERNAL_SAVE:
			ObCloseHandle(Context->Data.FileSave.FileHandle, KernelMode);
			break;
	}

	ExFreePoolWithTag(Context, RAMDISK_TAG);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

static void _InternalIOCTLCompletion(_In_ WDFREQUEST Request, _In_ WDFIOTARGET Target, _In_ PWDF_REQUEST_COMPLETION_PARAMS Params, _In_ WDFCONTEXT Context)
{
	IO_STATUS_BLOCK iosb;
	PIIOCTL_COMPLETION_CONTEXT ctx = (PIIOCTL_COMPLETION_CONTEXT)Context;
	DEBUG_ENTER_FUNCTION("Request=0x%p; Target=0x%p; Params=0x%p; Context=0x%p", Request, Target, Params, Context);

	iosb = Params->IoStatus;
	WdfRequestCompleteWithInformation(ctx->UpperRequest, iosb.Status, iosb.Information);
	_IIOCTLContextFree(ctx);
	WdfObjectDelete(Request);
	WdfIoTargetClose(Target);
	WdfObjectDelete(Target);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


static NTSTATUS _SendInternalIOCTL(PREMBUS_DISK_INFORMATION DiskInfo, ULONG ControlCode, PVOID InputBuffer, SIZE_T InputBufferLength, PVOID OutputBuffer, SIZE_T OutputBufferLength, PIIOCTL_COMPLETION_CONTEXT CompletionContext, PULONG_PTR Information)
{
	WDFREQUEST request = NULL;
	WDFIOTARGET target = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DiskInfo=0x%p; ControlCode=0x%p; InputBuffer=0x%p; InputBufferLength=%Iu; OutputBuffer=0x%p; OutputBufferLength=%Iu; CompletionContext=0x%p; Information=0x%p", DiskInfo, ControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, CompletionContext, Information);

	status = IoAcquireRemoveLock(&DiskInfo->FDORemoveLock, (PVOID)ControlCode);
	if (NT_SUCCESS(status)) {
		status = WdfIoTargetCreate(DiskInfo->FDO, WDF_NO_OBJECT_ATTRIBUTES, &target);
		if (NT_SUCCESS(status)) {
			WDF_IO_TARGET_OPEN_PARAMS openParams;
			PDEVICE_OBJECT wdmDevice = WdfDeviceWdmGetDeviceObject(DiskInfo->FDO);

			WDF_IO_TARGET_OPEN_PARAMS_INIT_EXISTING_DEVICE(&openParams, wdmDevice);
			status = WdfIoTargetOpen(target, &openParams);
			if (NT_SUCCESS(status)) {
				status = WdfRequestCreate(WDF_NO_OBJECT_ATTRIBUTES, target, &request);
				if (NT_SUCCESS(status)) {
					WDFMEMORY inputMemory = NULL;
					WDF_OBJECT_ATTRIBUTES memoryAttributes;

					WDF_OBJECT_ATTRIBUTES_INIT(&memoryAttributes);
					memoryAttributes.ParentObject = request;
					if (InputBufferLength > 0)
						status = WdfMemoryCreatePreallocated(&memoryAttributes, InputBuffer, InputBufferLength, &inputMemory);

					if (NT_SUCCESS(status)) {
						WDFMEMORY outputMemory = NULL;

						if (OutputBufferLength > 0)
							status = WdfMemoryCreatePreallocated(&memoryAttributes, OutputBuffer, OutputBufferLength, &outputMemory);

						if (NT_SUCCESS(status)) {
							status = WdfIoTargetFormatRequestForInternalIoctl(target, request, ControlCode, inputMemory, NULL, outputMemory, NULL);
							if (NT_SUCCESS(status)) {
								WDF_REQUEST_SEND_OPTIONS sendOptions;

								WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions, 0);
								if (CompletionContext != NULL) {
									WdfRequestSetCompletionRoutine(request, _InternalIOCTLCompletion, CompletionContext);
									status = STATUS_PENDING;
								}
								else WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions, WDF_REQUEST_SEND_OPTION_SYNCHRONOUS);

								if (!WdfRequestSend(request, target, &sendOptions) || CompletionContext == NULL) {
									status = WdfRequestGetStatus(request);
									*Information = WdfRequestGetInformation(request);
								}
							}
						}
					}

					if (status != STATUS_PENDING)
						WdfObjectDelete(request);
				}

				if (status != STATUS_PENDING)
					WdfIoTargetClose(target);
			}

			if (status != STATUS_PENDING)
				WdfObjectDelete(target);
		}
	
		IoReleaseRemoveLock(&DiskInfo->FDORemoveLock, (PVOID)ControlCode);
	}

	DEBUG_EXIT_FUNCTION("0x%x, *Information=0x%p", status, *Information);
	return status;
}


VOID _PDOEvtInternalDeviceControl(_In_ WDFQUEUE Queue, _In_ WDFREQUEST Request, _In_ size_t OutputBufferLength, _In_ size_t InputBufferLength, _In_ ULONG IoControlCode)
{
	ULONG_PTR information = 0;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Queue=0x%p; Request=0x%p; OutputBufferLength=%Iu; InputBufferLength=%Iu; IoControlCode=0x%x", Queue, Request, OutputBufferLength, InputBufferLength, IoControlCode);

	UNREFERENCED_PARAMETER(InputBufferLength);

	switch (IoControlCode) {
		case IOCTL_REMBUS_PDO_GET_DISKINFO: {
			PREMBUS_DISK_INFORMATION *output = NULL;

			information = sizeof(PREMBUS_DISK_INFORMATION);
			status = WdfRequestRetrieveOutputBuffer(Request, information, (PVOID *)&output, &OutputBufferLength);
			if (NT_SUCCESS(status))
				*output = GetPDOQueueContext(Queue)->DeviceInfo;
		} break;
		default:
			status = STATUS_INVALID_DEVICE_REQUEST;
			break;
	}

	WdfRequestCompleteWithInformation(Request, status, information);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


/************************************************************************/
/*                      PUBLIC FUNCTIONS                                */
/************************************************************************/


NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
    WDF_DRIVER_CONFIG config;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPhat=\"%wZ\"", DriverObject, RegistryPath);
	DEBUG_IRQL_LESS_OR_EQUAL(PASSIVE_LEVEL);

	WDF_DRIVER_CONFIG_INIT(&config, RemBusEvtDeviceAdd);
    status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE);

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


VOID RemBusEvtDeviceContextCleanup(IN WDFOBJECT Device)
{
//	PREMDISK_DEVICE_EXTENSION pDeviceExtension = DeviceGetExtension(Device);
	PAGED_CODE();
	DEBUG_ENTER_FUNCTION("Device=0x%p", Device);
	DEBUG_IRQL_LESS_OR_EQUAL(PASSIVE_LEVEL);

	UNREFERENCED_PARAMETER(Device);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


NTSTATUS RemBusEvtDeviceAdd(IN WDFDRIVER Driver, IN PWDFDEVICE_INIT DeviceInit)
{
	WDF_OBJECT_ATTRIBUTES deviceAttributes;
	NTSTATUS status;
	WDFDEVICE device;
	WDF_OBJECT_ATTRIBUTES queueAttributes;
	WDF_IO_QUEUE_CONFIG ioQueueConfig;
	PREMDISK_BUS_DEVICE_CONTEXT pDeviceExtension = NULL;
	PREMDISK_BUS_QUEUE_EXTENSION pQueueContext = NULL;
	WDFQUEUE queue;
	DECLARE_CONST_UNICODE_STRING(ntDeviceName, NT_DEVICE_NAME);
	PAGED_CODE();
	UNREFERENCED_PARAMETER(Driver);
	DEBUG_ENTER_FUNCTION("Driver=0x%p; DeviceInit=0x%p", Driver, DeviceInit);
	DEBUG_IRQL_LESS_OR_EQUAL(PASSIVE_LEVEL);

	status = WdfDeviceInitAssignName(DeviceInit, &ntDeviceName);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_UNKNOWN);
	WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoDirect);
	WdfDeviceInitSetExclusive(DeviceInit, FALSE);

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, REMDISK_BUS_DEVICE_CONTEXT);
	deviceAttributes.EvtCleanupCallback = RemBusEvtDeviceContextCleanup;
	deviceAttributes.ExecutionLevel = WdfExecutionLevelPassive;

	WDF_CHILD_LIST_CONFIG childListConfig;
	WDF_CHILD_LIST_CONFIG_INIT(&childListConfig, sizeof(PDO_IDENTIFICATION_DESCRIPTION), RemBusEvtDeviceListCreatePdo);
	WdfFdoInitSetDefaultChildListConfig(DeviceInit, &childListConfig, WDF_NO_OBJECT_ATTRIBUTES);
	status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	pDeviceExtension = GetBusDeviceContext(device);
	pDeviceExtension->DiskCount = 0;
	status = ExInitializeResourceLite(&pDeviceExtension->DiskListLock);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	for (size_t i = 0; i < sizeof(pDeviceExtension->Disks) / sizeof(pDeviceExtension->Disks[0]); ++i)
		pDeviceExtension->Disks[i] = NULL;

	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchParallel);
	ioQueueConfig.EvtIoDeviceControl = RemBusEvtIoDeviceControl;
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&queueAttributes, REMDISK_BUS_QUEUE_EXTENSION);

	__analysis_assume(ioQueueConfig.EvtIoStop != 0);
	status = WdfIoQueueCreate(device, &ioQueueConfig, &queueAttributes, &queue);
	__analysis_assume(ioQueueConfig.EvtIoStop == 0);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	pQueueContext = BusQueueGetExtension(queue);
	pQueueContext->DeviceExtension = pDeviceExtension;
	RtlInitUnicodeString(&pDeviceExtension->SymbolicLink, DOS_DEVICE_NAME);
	status = WdfDeviceCreateSymbolicLink(device, &pDeviceExtension->SymbolicLink);
	if (!NT_SUCCESS(status))
		goto Cleanup;

Cleanup:
	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


VOID RemBusEvtIoDeviceControl(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t OutputBufferLength, IN size_t InputBufferLength, IN ULONG IoControlCode)
{
	ULONG_PTR information = 0;
	NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
	PREMDISK_BUS_DEVICE_CONTEXT devExt = BusQueueGetExtension(Queue)->DeviceExtension;
	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);
	DEBUG_ENTER_FUNCTION("Queue=0x%p; Request=0x%p; OutputBufferLength=%Iu; InputBufferLength=%Iu; IoControlCode=0x%x", Queue, Request, OutputBufferLength, InputBufferLength, IoControlCode);
	DEBUG_IRQL_LESS_OR_EQUAL(PASSIVE_LEVEL);

	switch (IoControlCode) {
		case IOCTL_REMBUS_INFO: {
			size_t outputSize = 0;
			PIOCTL_REMBUS_INFO_OUTPUT output = NULL;

			information = sizeof(IOCTL_REMBUS_INFO_OUTPUT);
			status = WdfRequestRetrieveOutputBuffer(Request, information, &output, &outputSize);
			if (NT_SUCCESS(status)) {
				output->DriversInfo.BusMajorVersion = 1;
				output->DriversInfo.BusMinorVersion = 0;
				output->DriversInfo.DiskMajorVersion = 1;
				output->DriversInfo.DiskMinorVersion = 0;
				output->DriversInfo.Flags =
					REMBUS_FLAG_SUPPORTS_PLAIN_FILE_DISKS |
					REMBUS_FLAG_SUPPORTS_PLAIN_RAM_DISKS |
					REMBUS_FLAG_SUPPORTS_EF_FILE_DISKS |
					REMBUS_FLAG_SUPPORTS_EF_RAM_DISKS |
					REMBUS_FLAG_SUPPORTS_ENCRYPTED_FILE_DISKS |
					REMBUS_FLAG_SUPPORTS_ENCRYPTED_RAM_DISKS;
			}

			WdfRequestCompleteWithInformation(Request, status, information);
		} break;
		case IOCTL_REMBUS_DISK_CREATE: {
			size_t inputLen = 0;
			PIOCTL_REMBUS_DISK_CREATE_INPUT input = NULL;

			status = WdfRequestRetrieveInputBuffer(Request, sizeof(IOCTL_REMBUS_DISK_CREATE_INPUT), &input, &inputLen);
			if (NT_SUCCESS(status)) {
				PREMBUS_DISK_INFORMATION diskInfo = NULL;

				diskInfo = (PREMBUS_DISK_INFORMATION)ExAllocatePoolWithTag(NonPagedPool, sizeof(REMBUS_DISK_INFORMATION), RAMDISK_TAG);
				if (diskInfo != NULL) {
					diskInfo->ReferenceCount = 1;
					diskInfo->DiskNumber = input->DiskNumber;
					diskInfo->DiskSize = input->ImageSize.QuadPart;
					diskInfo->MaxTranfserLength = 2 * 1024 * 1024;
					diskInfo->FDOAttachedStatus = STATUS_UNSUCCESSFUL;
					KeInitializeEvent(&diskInfo->FDOAttachedEvent, NotificationEvent, FALSE);
					diskInfo->Type = input->Type;
					RtlCopyMemory(diskInfo->Password, input->Password, sizeof(diskInfo->Password));
					diskInfo->BackingFileObject = NULL;
					diskInfo->FDO = NULL;
					diskInfo->Flags = input->Flags;
					diskInfo->Inserted = TRUE;
					if (input->FileHandle != NULL) {
						IO_STATUS_BLOCK iosb;
						FILE_STANDARD_INFORMATION fsi;

						status = ZwQueryInformationFile(input->FileHandle, &iosb, &fsi, sizeof(fsi), FileStandardInformation);
						if (NT_SUCCESS(status)) {
							ACCESS_MASK desiredAccess = FILE_READ_DATA | SYNCHRONIZE;

							diskInfo->DiskSize = fsi.EndOfFile.QuadPart;
							if (FlagOn(REMDISK_FLAG_WRITABLE, diskInfo->Flags))
								desiredAccess |= FILE_WRITE_DATA;

							status = ObReferenceObjectByHandle(input->FileHandle, desiredAccess, *IoFileObjectType, WdfRequestGetRequestorMode(Request), &diskInfo->BackingFileObject, NULL);
						}
					} else if (diskInfo->Type == rdtRAMDisk) {
						if (FlagOn(REMDISK_FLAG_ENCRYPTED_FOOTER, diskInfo->Flags))
							diskInfo->DiskSize += sizeof(REMDISK_ENCRYPTED_FOOTER);
					} else if (diskInfo->Type == rdtFileDisk)
						status = STATUS_INVALID_PARAMETER;

					if (NT_SUCCESS(status)) {
						status = RemBusChildInsert(devExt, diskInfo);
						if (NT_SUCCESS(status)) {
							status = RemBusEnumerateChildren(WdfIoQueueGetDevice(Queue));
							if (NT_SUCCESS(status)) {
								(VOID)KeWaitForSingleObject(&diskInfo->FDOAttachedEvent, Executive, WdfRequestGetRequestorMode(Request), FALSE, NULL);
								status = diskInfo->FDOAttachedStatus;
								if (!NT_SUCCESS(status)) {
									diskInfo->Inserted = FALSE;
									RemBusEnumerateChildren(WdfIoQueueGetDevice(Queue));
								}
							}

							if (!NT_SUCCESS(status))
								RemBusChildRemove(devExt, diskInfo);
						}

						if (diskInfo->BackingFileObject != NULL) {
							ObDereferenceObject(diskInfo->BackingFileObject);
							diskInfo->BackingFileObject = NULL;
						}
					}

					if (!NT_SUCCESS(status))
						ExFreePoolWithTag(diskInfo, RAMDISK_TAG);
				} else status = STATUS_INSUFFICIENT_RESOURCES;
			}

			WdfRequestComplete(Request, status);
		} break;
		case IOCTL_REMBUS_DISK_PASSWORD_CHANGE: {
			size_t inputLen = 0;
			PIOCTL_REMBUS_DISK_PASSWORD_CHANGE_INPUT input = NULL;

			status = WdfRequestRetrieveInputBuffer(Request, sizeof(IOCTL_REMBUS_DISK_PASSWORD_CHANGE_INPUT), &input, &inputLen);
			if (NT_SUCCESS(status)) {
				PREMBUS_DISK_INFORMATION diskInfo = NULL;

				status = RemBusChildFindByDiskNumber(devExt, input->DiskNumber, FALSE, &diskInfo);
				if (NT_SUCCESS(status)) {
					PIIOCTL_COMPLETION_CONTEXT ctx = NULL;
					
					status = _IIOCTLContextAlloc(IOCTL_REMDISK_INTERNAL_PASSWORD, Request, &ctx);
					if (NT_SUCCESS(status)) {
						status = _SendInternalIOCTL(diskInfo, IOCTL_REMDISK_INTERNAL_PASSWORD, input, sizeof(IOCTL_REMBUS_DISK_PASSWORD_CHANGE_INPUT), NULL, 0, ctx, &information);
						if (!NT_SUCCESS(status))
							_IIOCTLContextFree(ctx);
					}
				}

			}
		} break;
		case IOCTL_REMBUS_DISK_LOAD: {
			size_t inputLen = 0;
			PIOCTL_REMBUS_DISK_LOAD_INPUT input = NULL;

			status = WdfRequestRetrieveInputBuffer(Request, sizeof(IOCTL_REMBUS_DISK_LOAD_INPUT), &input, &inputLen);
			if (NT_SUCCESS(status)) {
				PREMBUS_DISK_INFORMATION diskInfo = NULL;

				status = RemBusChildFindByDiskNumber(devExt, input->DiskNumber, FALSE, &diskInfo);
				if (NT_SUCCESS(status)) {
					PFILE_OBJECT fileObject = NULL;

					status = ObReferenceObjectByHandle(input->FileHandle, FILE_WRITE_DATA | SYNCHRONIZE, *IoFileObjectType, WdfRequestGetRequestorMode(Request), &fileObject, NULL);
					if (NT_SUCCESS(status)) {
						HANDLE newHandle = NULL;

						status = ObOpenObjectByPointer(fileObject, OBJ_KERNEL_HANDLE, NULL, FILE_READ_DATA | SYNCHRONIZE, *IoFileObjectType, KernelMode, &newHandle);
						if (NT_SUCCESS(status)) {
							PIIOCTL_COMPLETION_CONTEXT ctx = NULL;

							status = _IIOCTLContextAlloc(IOCTL_REMDISK_INTERNAL_LOAD, Request, &ctx);
							if (NT_SUCCESS(status)) {
								ctx->Data.FileLoad.FileHandle = newHandle;
								status = _SendInternalIOCTL(diskInfo, IOCTL_REMDISK_INTERNAL_LOAD, &newHandle, sizeof(newHandle), NULL, 0, ctx, &information);
								if (!NT_SUCCESS(status))
									_IIOCTLContextFree(ctx);
							}
						}

						ObDereferenceObject(fileObject);
					}
				}
			}
		} break;
		case IOCTL_REMBUS_DISK_SAVE: {
			size_t inputLen = 0;
			PIOCTL_REMBUS_DISK_SAVE_INPUT input = NULL;

			status = WdfRequestRetrieveInputBuffer(Request, sizeof(IOCTL_REMBUS_DISK_SAVE_INPUT), &input, &inputLen);
			if (NT_SUCCESS(status)) {
				PREMBUS_DISK_INFORMATION diskInfo = NULL;

				status = RemBusChildFindByDiskNumber(devExt, input->DiskNumber, FALSE, &diskInfo);
				if (NT_SUCCESS(status)) {
					PFILE_OBJECT fileObject = NULL;

					status = ObReferenceObjectByHandle(input->FileHandle, FILE_WRITE_DATA | SYNCHRONIZE, *IoFileObjectType, WdfRequestGetRequestorMode(Request), &fileObject, NULL);
					if (NT_SUCCESS(status)) {
						HANDLE newHandle = NULL;

						status = ObOpenObjectByPointer(fileObject, OBJ_KERNEL_HANDLE, NULL, FILE_WRITE_DATA | SYNCHRONIZE, *IoFileObjectType, KernelMode, &newHandle);
						if (NT_SUCCESS(status)) {
							PIIOCTL_COMPLETION_CONTEXT ctx = NULL;
							
							status = _IIOCTLContextAlloc(IOCTL_REMDISK_INTERNAL_SAVE, Request, &ctx);
							if (NT_SUCCESS(status)) {
								ctx->Data.FileSave.FileHandle = newHandle;
								status = _SendInternalIOCTL(diskInfo, IOCTL_REMDISK_INTERNAL_SAVE, &newHandle, sizeof(newHandle), NULL, 0, ctx, &information);
								if (!NT_SUCCESS(status))
									_IIOCTLContextFree(ctx);
							}
						}

						ObDereferenceObject(fileObject);
					}
				}
			}
		} break;
		case IOCTL_REMBUS_DISK_REMOVE: {
			size_t inputLen = 0;
			PIOCTL_REMBUS_DISK_REMOVE_INPUT input = NULL;

			status = WdfRequestRetrieveInputBuffer(Request, sizeof(IOCTL_REMBUS_DISK_REMOVE_INPUT), &input, &inputLen);
			if (NT_SUCCESS(status)) {
				PREMBUS_DISK_INFORMATION diskInfo = NULL;

				status = RemBusChildFindByDiskNumber(devExt, input->DiskNumber, FALSE, &diskInfo);
				if (NT_SUCCESS(status)) {
					if (diskInfo->Inserted) {
						diskInfo->Inserted = FALSE;
						status = RemBusEnumerateChildren(WdfIoQueueGetDevice(Queue));
					}

					if (NT_SUCCESS(status)) {
						if (diskInfo->BackingFileObject != NULL)
							ObDereferenceObject(diskInfo->BackingFileObject);

						if (diskInfo->FDO != NULL)
							WdfObjectDereference(diskInfo->FDO);

						if (InterlockedDecrement(&diskInfo->ReferenceCount) == 0) {
							RemBusChildRemove(devExt, diskInfo);
							ExFreePoolWithTag(diskInfo, RAMDISK_TAG);
						}
					}
				}
			}

			WdfRequestComplete(Request, status);
		} break;
		case IOCTL_REMBUS_DISK_GET_SETTINGS: {
			size_t inputLen = 0;
			PIOCTL_REMBUS_DISK_GET_SETTINGS_INPUT input = NULL;

			information = 0;
			status = WdfRequestRetrieveInputBuffer(Request, sizeof(IOCTL_REMBUS_DISK_GET_SETTINGS_INPUT), &input, &inputLen);
			if (NT_SUCCESS(status)) {
				PREMBUS_DISK_INFORMATION diskInfo = NULL;

				status = RemBusChildFindByDiskNumber(devExt, input->DiskNumber, FALSE, &diskInfo);
				if (NT_SUCCESS(status)) {
					PIOCTL_REMBUS_DISK_GET_SETTINGS_OUTPUT output = NULL;

					status = WdfRequestRetrieveOutputBuffer(Request, sizeof(IOCTL_REMBUS_DISK_GET_SETTINGS_OUTPUT), &output, &OutputBufferLength);
					if (NT_SUCCESS(status))
						status = _SendInternalIOCTL(diskInfo, IOCTL_REMDISK_INTERNAL_QUERY, NULL, 0, &output->Entry, OutputBufferLength - FIELD_OFFSET(IOCTL_REMBUS_DISK_GET_SETTINGS_OUTPUT, Entry), NULL, &information);
				}
			}

			WdfRequestCompleteWithInformation(Request, status, information);
		} break;
		case IOCTL_REMBUS_DISK_ENUMERATE: {
			size_t outputSize = 0;
			PIOCTL_REMBUS_DISK_ENUMERATE_OUTPUT output = NULL;
		
			status = WdfRequestRetrieveOutputBuffer(Request, sizeof(IOCTL_REMBUS_DISK_ENUMERATE_OUTPUT), &output, &outputSize);
			if (NT_SUCCESS(status)) {
				size_t totalLength = FIELD_OFFSET(IOCTL_REMBUS_DISK_ENUMERATE_OUTPUT, Entry);
				PREMDISK_INFORMATION_ENTRY entry = &output->Entry;

				output->Count = 0;
				RemBusChildListLock(devExt, TRUE);
				for (size_t i = 0; i < sizeof(devExt->Disks) / sizeof(devExt->Disks[0]); ++i) {
					PREMBUS_DISK_INFORMATION di = devExt->Disks[i];

					if (di != NULL) {
						status = _SendInternalIOCTL(di, IOCTL_REMDISK_INTERNAL_QUERY, NULL, 0, entry, outputSize - totalLength, NULL, &information);
						if (NT_SUCCESS(status)) {
							totalLength += entry->NextEntryOffset;
							entry = (PREMDISK_INFORMATION_ENTRY)((PUCHAR)entry + entry->NextEntryOffset);
							++output->Count;
						}

						if (status == STATUS_DELETE_PENDING) {
							if (totalLength + sizeof(REMDISK_INFORMATION_ENTRY) <= outputSize) {
								entry->NextEntryOffset = sizeof(REMDISK_INFORMATION_ENTRY);
								entry->DiskNumber = di->DiskNumber;
								entry->DiskSize.QuadPart = di->DiskSize;
								entry->FileNameLength = 0;
								entry->FileNameOffset = sizeof(REMDISK_INFORMATION_ENTRY);
								entry->Flags = di->Flags;
								entry->ParentDiskNumber = 0;
								entry->State = rdisRemoved;
								entry->Type = di->Type;
								++output->Count;
								totalLength += entry->NextEntryOffset;
								entry = (PREMDISK_INFORMATION_ENTRY)((PUCHAR)entry + entry->NextEntryOffset);
								status = STATUS_SUCCESS;
							} else status = STATUS_BUFFER_TOO_SMALL;
						}
					}

					if (!NT_SUCCESS(status))
						break;
				}

				if (NT_SUCCESS(status))
					information = totalLength;

				RemBusChildListUnlock(devExt);
			}

			WdfRequestCompleteWithInformation(Request, status, information);
		} break;
		case IOCTL_REMBUS_DISK_SET_SETTINGS: {
			size_t inputLen = 0;
			PIOCTL_REMBUS_DISK_SET_SETTINGS_INPUT input = NULL;

			status = WdfRequestRetrieveInputBuffer(Request, sizeof(IOCTL_REMBUS_DISK_SET_SETTINGS_INPUT), &input, &inputLen);
			if (NT_SUCCESS(status)) {
				PREMBUS_DISK_INFORMATION diskInfo = NULL;

				status = RemBusChildFindByDiskNumber(devExt, input->DiskNumber, FALSE, &diskInfo);
				if (NT_SUCCESS(status)) {

				}
			}

			WdfRequestComplete(Request, status);
		} break;
		default:
			status = STATUS_INVALID_DEVICE_REQUEST;
			WdfRequestComplete(Request, status);
			break;
	}
	
	DEBUG_EXIT_FUNCTION("void, status=0x%x, information=%Iu", status, information);
	return;
}


_IRQL_requires_max_(APC_LEVEL)
NTSTATUS RemBusEnumerateChildren(_In_ WDFDEVICE Device)
{
	WDFCHILDLIST list = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PREMDISK_BUS_DEVICE_CONTEXT pDeviceContext;
	DEBUG_ENTER_FUNCTION("Device=0x%p", Device);
	DEBUG_IRQL_LESS_OR_EQUAL(APC_LEVEL);

	status = STATUS_SUCCESS;
	pDeviceContext = GetBusDeviceContext(Device);
	list = WdfFdoGetDefaultChildList(Device);
	WdfChildListBeginScan(list);
//	WdfChildListUpdateAllChildDescriptionsAsPresent(list);
	RemBusChildListLock(pDeviceContext, TRUE);
	for (size_t i = 0; i < sizeof(pDeviceContext->Disks) / sizeof(pDeviceContext->Disks[0]); ++i) {
		PREMBUS_DISK_INFORMATION di = pDeviceContext->Disks[i];
		PDO_IDENTIFICATION_DESCRIPTION description;

		if (di == NULL)
			continue;

		WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(&description.Header, sizeof(description));
		description.DiskNumber = di->DiskNumber;
		RtlSecureZeroMemory(description.HardwareId, sizeof(description.HardwareId));
		switch (di->Type) {
			case rdtRAMDisk:
				memcpy(description.HardwareId, RAMDISK_DEVICE_ID, sizeof(RAMDISK_DEVICE_ID));
				break;
			case rdtFileDisk:
				memcpy(description.HardwareId, FILEDISK_DEVICE_ID, sizeof(FILEDISK_DEVICE_ID));
				break;
			default:
				ASSERT(FALSE);
				break;
		}

		if (di->Inserted)
			status = WdfChildListAddOrUpdateChildDescriptionAsPresent(list, &description.Header, NULL);
	}

	RemBusChildListUnlock(pDeviceContext);
	WdfChildListEndScan(list);

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


VOID RemBusPDOEvtDeviceContextCleanup(WDFDEVICE Device)
{
	DEBUG_EXIT_FUNCTION("Device=0x%p", Device);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


NTSTATUS RemBusEvtDeviceListCreatePdo(WDFCHILDLIST DeviceList, PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription, PWDFDEVICE_INIT ChildInit)
{
	WDFDEVICE hChild = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	WDF_OBJECT_ATTRIBUTES deviceAttributes;
	PPDO_IDENTIFICATION_DESCRIPTION pDesc = NULL;
	PREMBUS_PDO_DEVICE_CONTEXT pDeviceExtension = NULL;
	PREMDISK_BUS_DEVICE_CONTEXT busExtension = NULL;

	DECLARE_UNICODE_STRING_SIZE(deviceLocation, 32);
	DECLARE_UNICODE_STRING_SIZE(buffer, 64);
	UNICODE_STRING uDeviceId;
	DEBUG_ENTER_FUNCTION("DeviceList=0x%p; IdentificationDescription=0x%p; ChildInit=0x%p", DeviceList, IdentificationDescription, ChildInit);
	DEBUG_IRQL_LESS_OR_EQUAL(APC_LEVEL);

	pDesc = CONTAINING_RECORD(IdentificationDescription, PDO_IDENTIFICATION_DESCRIPTION, Header);
	busExtension = GetBusDeviceContext(WdfChildListGetDevice(DeviceList));
	PREMBUS_DISK_INFORMATION diskInfo = NULL;
	status = RemBusChildFindByDiskNumber(busExtension, pDesc->DiskNumber, FALSE, &diskInfo);
	if (!NT_SUCCESS(status))
		goto Cleanup;
	
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, REMBUS_PDO_DEVICE_CONTEXT);
	deviceAttributes.EvtCleanupCallback = RemBusPDOEvtDeviceContextCleanup;
	deviceAttributes.ExecutionLevel = WdfExecutionLevelPassive;

	RtlInitUnicodeString(&uDeviceId, pDesc->HardwareId);
	status = WdfPdoInitAssignDeviceID(ChildInit, &uDeviceId);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	status = WdfPdoInitAddHardwareID(ChildInit, &uDeviceId);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	RtlInitUnicodeString(&uDeviceId, REMDISK_DEVICE_ID);
	status = WdfPdoInitAddCompatibleID(ChildInit, &uDeviceId);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	status = RtlUnicodeStringPrintf(&buffer, L"%02d", pDesc->DiskNumber);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	status = WdfPdoInitAssignInstanceID(ChildInit, &buffer);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	status = RtlUnicodeStringPrintf(&buffer, L"REMDisk Disk Device %02d", pDesc->DiskNumber);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	status = RtlUnicodeStringPrintf(&deviceLocation, L"RemBus_%02d", pDesc->DiskNumber);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	status = WdfPdoInitAddDeviceText(ChildInit, &buffer, &deviceLocation, 0x409);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	WdfPdoInitSetDefaultLocale(ChildInit, 0x409);
	status = WdfDeviceCreate(&ChildInit, &deviceAttributes, &hChild);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	pDeviceExtension = GetPDODeviceContext(hChild);
	pDeviceExtension->DeviceInfo = diskInfo;

	WDF_DEVICE_PNP_CAPABILITIES pnpCapabilities;
	WDF_DEVICE_PNP_CAPABILITIES_INIT(&pnpCapabilities);
	pnpCapabilities.Address = pDesc->DiskNumber;
	pnpCapabilities.Removable = WdfTrue;
	pnpCapabilities.DockDevice = WdfFalse;
	pnpCapabilities.EjectSupported = WdfTrue;
	pnpCapabilities.HardwareDisabled = WdfFalse;
	pnpCapabilities.LockSupported = WdfTrue;
	pnpCapabilities.NoDisplayInUI = WdfFalse;
	pnpCapabilities.SurpriseRemovalOK = WdfFalse;
	pnpCapabilities.UINumber = pDesc->DiskNumber;
	pnpCapabilities.UniqueID = WdfFalse;
	WdfDeviceSetPnpCapabilities(hChild, &pnpCapabilities);

	WDFQUEUE queue;
	WDF_IO_QUEUE_CONFIG ioQueueConfig;
	WDF_OBJECT_ATTRIBUTES queueAttributes;
	PREMBUS_PDO_QUEUE_CONTEXT pQueueContext = NULL;

	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchParallel);
	ioQueueConfig.PowerManaged = WdfFalse;
	ioQueueConfig.EvtIoInternalDeviceControl = _PDOEvtInternalDeviceControl;
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&queueAttributes, REMBUS_PDO_QUEUE_CONTEXT);
	__analysis_assume(ioQueueConfig.EvtIoStop != 0);
	status = WdfIoQueueCreate(hChild, &ioQueueConfig, &queueAttributes, &queue);
	__analysis_assume(ioQueueConfig.EvtIoStop == 0);
	if (!NT_SUCCESS(status))
		goto Cleanup;

	pQueueContext = GetPDOQueueContext(queue);
	pQueueContext->DeviceInfo = diskInfo;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
Cleanup:
	if (diskInfo != NULL) {
		diskInfo->FDOAttachedStatus = status;
		KeSetEvent(&diskInfo->FDOAttachedEvent, IO_NO_INCREMENT, FALSE);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}
