
#include <ntifs.h>
#include <wdf.h>
#include "preprocessor.h"
#include "remdisk-device.h"
#include "remdisk.h"






NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	WDF_DRIVER_CONFIG config;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPhat=\"%wZ\"", DriverObject, RegistryPath);
	DEBUG_IRQL_LESS_OR_EQUAL(PASSIVE_LEVEL);

	WDF_DRIVER_CONFIG_INIT(&config, RemDiskEtvDeviceAdd);
	status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE);

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


