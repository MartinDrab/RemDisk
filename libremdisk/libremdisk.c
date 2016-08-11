
#include <windows.h>
#include <newdev.h>
#include <SetupAPI.h>
#include <initguid.h>
#include "general-types.h"
#include "ioctls.h"
#include "libremdisk.h"


/************************************************************************/
/*                 GLOBAL VARIABLES                                     */
/************************************************************************/

static HANDLE _deviceHandle = INVALID_HANDLE_VALUE;
static HANDLE _remDiskHeap = NULL;

static const WCHAR *_busClassName = L"RemBus";
static const WCHAR *_diskClassName = L"RemDisk";
// {78A1C341-4539-11d3-B88D-00C04FAD5171}
DEFINE_GUID(_busClassGuid, 0x78A1C341, 0x4539, 0x11d3, 0xB8, 0x8D, 0x00, 0xC0, 0x4F, 0xAD, 0x51, 0x71);
static const WCHAR *_busHardwareIds = L"Root\\RemBus\0";

/************************************************************************/
/*                 HELPER FUNCTIONS                                     */
/************************************************************************/

static DWORD _SynchronousIOCTLWriteRequest(ULONG ControlCode, PVOID InputBuffer, ULONG InputBufferLength)
{
	DWORD dummy = 0;

	return (DeviceIoControl(_deviceHandle, ControlCode, InputBuffer, InputBufferLength, NULL, 0, &dummy, NULL) ? ERROR_SUCCESS : GetLastError());
}


static DWORD _SynchronousIOCTLReadRequest(ULONG ControlCode, PVOID OutputBuffer, ULONG OutputBufferLength)
{
	DWORD dummy = 0;

	return (DeviceIoControl(_deviceHandle, ControlCode, NULL, 0, OutputBuffer, OutputBufferLength, &dummy, NULL) ? ERROR_SUCCESS : GetLastError());
}


static DWORD _SynchronousIOCTLOtherRequest(ULONG ControlCode, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength)
{
	DWORD dummy = 0;

	return (DeviceIoControl(_deviceHandle, ControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, &dummy, NULL) ? ERROR_SUCCESS : GetLastError());
}


static DWORD _SynchronousIOCTLVariableLengthRequest(ULONG ControlCode, PVOID InputBuffer, ULONG InputBufferLength, PVOID *OutputBuffer)
{
	ULONG bufSize = 128;
	PVOID tmpBuf = NULL;
	DWORD ret = ERROR_GEN_FAILURE;

	do {
		tmpBuf = HeapAlloc(_remDiskHeap, HEAP_ZERO_MEMORY, bufSize);
		if (tmpBuf != NULL) {
			ret = _SynchronousIOCTLOtherRequest(ControlCode, InputBuffer, InputBufferLength, tmpBuf, bufSize);
			if (ret != ERROR_SUCCESS) {
				HeapFree(_remDiskHeap, 0, tmpBuf);
				bufSize *= 2;
			}
		} else ret = ERROR_NOT_ENOUGH_MEMORY;
	} while (ret == ERROR_INSUFFICIENT_BUFFER);

	if (ret == ERROR_SUCCESS)
		*OutputBuffer = tmpBuf;

	return ret;
}


static DWORD _EnumEntryToInfo(PREMDISK_INFORMATION_ENTRY Entry, PREMDISK_INFO Info)
{
	DWORD ret = ERROR_GEN_FAILURE;

	ret = ERROR_SUCCESS;
	Info->DiskNumber = Entry->DiskNumber;
	Info->DiskSize = Entry->DiskSize.QuadPart;
	Info->Flags = Entry->Flags;
	Info->State = Entry->State;
	Info->Type = Entry->Type;
	Info->FileName = (PWCHAR)HeapAlloc(_remDiskHeap, HEAP_ZERO_MEMORY, Entry->FileNameLength + sizeof(WCHAR));
	if (Info->FileName != NULL) {
		memcpy(Info->FileName, (PUCHAR)Entry + Entry->FileNameOffset, Entry->FileNameLength);
		Info->FileName[Entry->FileNameLength / sizeof(WCHAR)] = L'\0';
	} else ret = ERROR_NOT_ENOUGH_MEMORY;

	return ret;
}


static DWORD _CreateDevice(const WCHAR *InfPath, const WCHAR *ClassName, const GUID *ClassGuid, const WCHAR *HardwareIds)
{
	HDEVINFO DeviceInfoSet = INVALID_HANDLE_VALUE;
	SP_DEVINFO_DATA DeviceInfoData;
	DWORD ret = ERROR_GEN_FAILURE;
	DWORD hardwareIdSize = 1;
	const WCHAR *hwId = HardwareIds;
	
	while (*hwId != L'\0') {
		size_t len = wcslen(hwId);

		hardwareIdSize += (len + 1);
		hwId += (len + 1);
	}

	hardwareIdSize *= 2;

	//
	// Create the container for the to-be-created Device Information Element.
	//
	DeviceInfoSet = SetupDiCreateDeviceInfoList(ClassGuid, 0);
	ret = (DeviceInfoSet != INVALID_HANDLE_VALUE) ? ERROR_SUCCESS : GetLastError();
	if (ret == ERROR_SUCCESS) {
		//
		// Now create the element.
		// Use the Class GUID and Name from the INF file.
		//
		DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
		ret = (SetupDiCreateDeviceInfoW(DeviceInfoSet, ClassName, ClassGuid, NULL, 0, DICD_GENERATE_ID, &DeviceInfoData)) ? ERROR_SUCCESS : GetLastError();
		if (ret == ERROR_SUCCESS) {
			//
			// Add the HardwareID to the Device's HardwareID property.
			//
			ret = (SetupDiSetDeviceRegistryPropertyW(DeviceInfoSet, &DeviceInfoData, SPDRP_HARDWAREID, (LPBYTE)HardwareIds, hardwareIdSize)) ? ERROR_SUCCESS : GetLastError();
			if (ret == ERROR_SUCCESS) {
				//
				// Transform the registry element into an actual devnode
				// in the PnP HW tree.
				//
				ret = (SetupDiCallClassInstaller(DIF_REGISTERDEVICE, DeviceInfoSet, &DeviceInfoData)) ? ERROR_SUCCESS : GetLastError();
				if (ret == ERROR_SUCCESS) {
					ret = (UpdateDriverForPlugAndPlayDevices(NULL, HardwareIds, InfPath, INSTALLFLAG_FORCE, NULL)) ? ERROR_SUCCESS : GetLastError();
				}
			}
		}

		SetupDiDestroyDeviceInfoList(DeviceInfoSet);
	}

	return ret;
}

/************************************************************************/
/*                    PUBLIC FUNCTIONS                                  */
/************************************************************************/

DWORD RemDiskInstall(const WCHAR *BusDriverINFPath)
{
	DWORD ret = ERROR_GEN_FAILURE;

	ret = _CreateDevice(BusDriverINFPath, _busClassName, &_busClassGuid, _busHardwareIds);

	return ret;
}

DWORD RemDiskCreate(ULONG DiskNumber, EREMDiskType Type, ULONG64 DiskSize, const wchar_t *FileName, ULONG ShareMode, ULONG Flags, const void *Password, size_t PasswordLength)
{
	DWORD ret = ERROR_GEN_FAILURE;
	IOCTL_REMBUS_DISK_CREATE_INPUT input;

	ret = ERROR_SUCCESS;
	memset(&input, 0, sizeof(input));
	if (Type == rdtRAMDisk || (FileName != NULL && *FileName != L'\0')) {
		ret = ERROR_SUCCESS;
		input.FileHandle = NULL;
		if (Type == rdtFileDisk && FileName != NULL && *FileName != L'\0') {
			DWORD shareMode = 0;

			if (ShareMode & FILE_SHARE_READ)
				shareMode = FILE_SHARE_READ;

			input.FileHandle = CreateFileW(FileName, GENERIC_READ | GENERIC_WRITE, shareMode, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
			ret = (input.FileHandle != INVALID_HANDLE_VALUE) ? ERROR_SUCCESS : GetLastError();
			if (ret == ERROR_SUCCESS) {
				if (DiskSize > 64 * 1024 * 1024) {
					if (Flags & REMDISK_FLAG_SPARSE_FILE) {
						DWORD dummy = 0;

						ret = (DeviceIoControl(input.FileHandle, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &dummy, NULL)) ? ERROR_SUCCESS : GetLastError();
					}

					if (ret == ERROR_SUCCESS) {
						LARGE_INTEGER remaining;
						ULONG chunkSize = 2 * 1024 * 1024;
						PVOID buf = NULL;

						buf = HeapAlloc(_remDiskHeap, HEAP_ZERO_MEMORY, chunkSize);
						if (buf != NULL) {
							if (Flags & REMDISK_FLAG_ENCRYPTED_FOOTER)
								DiskSize += 4096;

							remaining.QuadPart = DiskSize;
							while (ret == ERROR_SUCCESS && remaining.QuadPart > 0) {
								ULONG bytesWritten = 0;

								if (remaining.QuadPart < chunkSize)
									chunkSize = remaining.LowPart;

								ret = (WriteFile(input.FileHandle, buf, chunkSize, &bytesWritten, NULL)) ? ERROR_SUCCESS : GetLastError();
								if (ret == ERROR_SUCCESS)
									remaining.QuadPart -= chunkSize;
							}

							HeapFree(_remDiskHeap, 0, buf);
						} else ret = ERROR_NOT_ENOUGH_MEMORY;
					}
				} else ret = ERROR_INVALID_PARAMETER;
			}
		}

		if (ret == ERROR_SUCCESS) {
			if (Flags & REMDISK_FLAG_ENCRYPTED) {
				if (PasswordLength > 0 &&PasswordLength <= sizeof(input.Password) - sizeof(ULONG)) {
					memcpy(input.Password, &PasswordLength, sizeof(ULONG));
					memcpy(input.Password + sizeof(ULONG), Password, PasswordLength);
				} else ret = ERROR_INVALID_PARAMETER;
			}

			if (ret == ERROR_SUCCESS) {
				input.DiskNumber = DiskNumber;
				input.Flags = Flags;
				input.Type = Type;
				input.ImageSize.QuadPart = DiskSize;
				ret = _SynchronousIOCTLWriteRequest(IOCTL_REMBUS_DISK_CREATE, &input, sizeof(input));
			}
		}

		if (input.FileHandle != NULL && input.FileHandle != INVALID_HANDLE_VALUE)
			CloseHandle(input.FileHandle);
	} else ret = ERROR_INVALID_PARAMETER;

	return ret;
}


DWORD RemDiskOpen(ULONG DiskNumber, EREMDiskType Type, const wchar_t *FileName, DWORD ShareMode, ULONG Flags, const unsigned char *Password, size_t PasswordLength)
{
	DWORD ret = ERROR_GEN_FAILURE;
	IOCTL_REMBUS_DISK_CREATE_INPUT input;

	ret = ERROR_SUCCESS;
	memset(&input, 0, sizeof(input));
	if (FileName != NULL && *FileName != L'\0') {
		if (Flags & REMDISK_FLAG_ENCRYPTED) {
			if (PasswordLength > 0 && PasswordLength <= sizeof(input.Password) - sizeof(ULONG)) {
				*(PULONG)input.Password = (ULONG)PasswordLength;
				memcpy(input.Password + sizeof(ULONG), Password, PasswordLength);
			} else ret = ERROR_INVALID_PARAMETER;
		}
		
		if (ret == ERROR_SUCCESS) {
			DWORD access = FILE_READ_DATA | SYNCHRONIZE;

			if (Flags & REMDISK_FLAG_WRITABLE)
				access |= FILE_WRITE_DATA;

			if (ShareMode & FILE_SHARE_READ)
				ShareMode = FILE_SHARE_READ;
			else ShareMode = 0;

			input.FileHandle = CreateFileW(FileName, access, ShareMode, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			ret = (input.FileHandle != INVALID_HANDLE_VALUE) ? ERROR_SUCCESS : GetLastError();
			if (ret == ERROR_SUCCESS) {
				Flags |= REMDISK_FLAG_FILE_SOURCE;
				input.DiskNumber = DiskNumber;
				input.Flags = Flags;
				input.Type = Type;
				ret = _SynchronousIOCTLWriteRequest(IOCTL_REMBUS_DISK_CREATE, &input, sizeof(input));
				CloseHandle(input.FileHandle);
			}
		}
	} else ret = ERROR_INVALID_PARAMETER;

	return ret;
}

DWORD RemDiskChangePassword(ULONG DiskNumber, const void *NewPassword, size_t NewPasswordLength, const void *OldPassword, size_t OldPasswordLength)
{
	DWORD ret = ERROR_GEN_FAILURE;
	IOCTL_REMBUS_DISK_PASSWORD_CHANGE_INPUT input;

	ret = ERROR_SUCCESS;
	memset(&input, 0, sizeof(input));
	input.DiskNumber = DiskNumber;
	input.ChangeType = rdpcChange;
	if (NewPasswordLength > 0 && NewPasswordLength <= sizeof(input.Password) - sizeof(ULONG)) {
		memcpy(input.Password, &NewPasswordLength, sizeof(ULONG));
		memcpy(input.Password + sizeof(ULONG), NewPassword, NewPasswordLength);
	} else ret = ERROR_INVALID_PARAMETER;

	if (ret == ERROR_SUCCESS) {
		if (OldPasswordLength > 0 && OldPasswordLength <= sizeof(input.OldPassword) - sizeof(ULONG)) {
			memcpy(input.OldPassword, &OldPasswordLength, sizeof(ULONG));
			memcpy(input.OldPassword + sizeof(ULONG), OldPassword, OldPasswordLength);
		} else ret = ERROR_INVALID_PARAMETER;
	}

	if (ret == ERROR_SUCCESS)
		ret = _SynchronousIOCTLWriteRequest(IOCTL_REMBUS_DISK_PASSWORD_CHANGE, &input, sizeof(input));

	return ret;
}


DWORD RemDiskEncrypt(ULONG DiskNumber, const void *Password, size_t PasswordLength, BOOLEAN Footer)
{
	DWORD ret = ERROR_GEN_FAILURE;
	IOCTL_REMBUS_DISK_PASSWORD_CHANGE_INPUT input;

	ret = ERROR_SUCCESS;
	memset(&input, 0, sizeof(input));
	input.DiskNumber = DiskNumber;
	input.ChangeType = (Footer) ? rdpcSetEncryptedFooter : rdpcSet;
	if (PasswordLength > 0 && PasswordLength <= sizeof(input.Password) - sizeof(ULONG)) {
		memcpy(input.Password, &PasswordLength, sizeof(ULONG));
		memcpy(input.Password + sizeof(ULONG), Password, PasswordLength);
	} else ret = ERROR_INVALID_PARAMETER;

	if (ret == ERROR_SUCCESS)
		ret = _SynchronousIOCTLWriteRequest(IOCTL_REMBUS_DISK_PASSWORD_CHANGE, &input, sizeof(input));

	return ret;
}


DWORD RemDiskDecrypt(ULONG DiskNumber, const void *Password, size_t PasswordLength)
{
	DWORD ret = ERROR_GEN_FAILURE;
	IOCTL_REMBUS_DISK_PASSWORD_CHANGE_INPUT input;

	ret = ERROR_SUCCESS;
	memset(&input, 0, sizeof(input));
	input.DiskNumber = DiskNumber;
	input.ChangeType = rdpcClear;
	if (PasswordLength > 0 && PasswordLength <= sizeof(input.Password) - sizeof(ULONG)) {
		memcpy(input.Password, &PasswordLength, sizeof(ULONG));
		memcpy(input.Password + sizeof(ULONG), Password, PasswordLength);
	} else ret = ERROR_INVALID_PARAMETER;

	if (ret == ERROR_SUCCESS)
		ret = _SynchronousIOCTLWriteRequest(IOCTL_REMBUS_DISK_PASSWORD_CHANGE, &input, sizeof(input));

	return ret;
}


DWORD RemDiskFree(ULONG DiskNumber)
{
	DWORD ret = ERROR_GEN_FAILURE;
	IOCTL_REMBUS_DISK_REMOVE_INPUT input;

	input.DiskNumber = DiskNumber;
	ret = _SynchronousIOCTLWriteRequest(IOCTL_REMBUS_DISK_REMOVE, &input, sizeof(input));

	return ret;
}


DWORD RemDiskEnumerate(PREMDISK_INFO *Array, PULONG Count)
{
	DWORD ret = ERROR_GEN_FAILURE;
	PIOCTL_REMBUS_DISK_ENUMERATE_OUTPUT output = NULL;

	ret = _SynchronousIOCTLVariableLengthRequest(IOCTL_REMBUS_DISK_ENUMERATE, NULL, 0, (PVOID *)&output);
	if (ret == ERROR_SUCCESS) {
		const ULONG count = output->Count;
		PREMDISK_INFO tmpInfo = NULL;

		tmpInfo = (PREMDISK_INFO)HeapAlloc(_remDiskHeap, HEAP_ZERO_MEMORY, count*sizeof(REMDISK_INFO));
		if (tmpInfo != NULL) {
			PREMDISK_INFORMATION_ENTRY entry = &output->Entry;

			for (size_t i = 0; i < count; ++i) {
				ret = _EnumEntryToInfo(entry, tmpInfo + i);
				if (ret != ERROR_SUCCESS) {
					for (size_t j = 0; j < i; ++j)
						RemDiskInfoFree(tmpInfo + j);

					break;
				}

				entry = (PREMDISK_INFORMATION_ENTRY)((PUCHAR)entry + entry->NextEntryOffset);
			}

			if (ret == ERROR_SUCCESS) {
				*Array = tmpInfo;
				*Count = count;
			}

			if (ret != ERROR_SUCCESS)
				HeapFree(_remDiskHeap, 0, tmpInfo);
		}

		HeapFree(_remDiskHeap, 0, output);
	}

	return ret;
}


VOID RemDiskEnumerationFree(PREMDISK_INFO Array, ULONG Count)
{
	for (size_t i = 0; i < Count; ++i)
		RemDiskInfoFree(Array + i);

	HeapFree(_remDiskHeap, 0, Array);

	return;
}


DWORD RemDiskGetInfo(ULONG DiskNumber, PREMDISK_INFO Info)
{
	DWORD ret = ERROR_GEN_FAILURE;
	IOCTL_REMBUS_DISK_GET_SETTINGS_INPUT input;
	PIOCTL_REMBUS_DISK_GET_SETTINGS_OUTPUT output = NULL;

	input.DiskNumber = DiskNumber;
	ret = _SynchronousIOCTLVariableLengthRequest(IOCTL_REMBUS_DISK_GET_SETTINGS, &input, sizeof(input), (PVOID *)&output);
	if (ret == ERROR_SUCCESS) {
		ret = _EnumEntryToInfo(&output->Entry, Info);
		HeapFree(_remDiskHeap, 0, output);
	}

	return ret;

}


VOID RemDiskInfoFree(PREMDISK_INFO Info)
{
	HeapFree(_remDiskHeap, 0, Info->FileName);

	return;
}


DWORD RemDiskLoadFile(ULONG DiskNumber, const wchar_t *FileName, DWORD ShareMode)
{
	DWORD ret = ERROR_GEN_FAILURE;
	IOCTL_REMBUS_DISK_LOAD_INPUT input;

	ret = ERROR_SUCCESS;
	input.DiskNumber = DiskNumber;
	input.FileHandle = CreateFileW(FileName, FILE_READ_DATA, ShareMode, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	ret = (input.FileHandle != INVALID_HANDLE_VALUE) ? ERROR_SUCCESS : GetLastError();
	if (ret == ERROR_SUCCESS)
		ret = _SynchronousIOCTLWriteRequest(IOCTL_REMBUS_DISK_LOAD, &input, sizeof(input));

	CloseHandle(input.FileHandle);

	return ret;
}


DWORD RemDiskSaveFile(ULONG DiskNumber, const wchar_t *FileName, DWORD ShareMode)
{
	DWORD ret = ERROR_GEN_FAILURE;
	IOCTL_REMBUS_DISK_SAVE_INPUT input;

	ret = ERROR_SUCCESS;
	input.DiskNumber = DiskNumber;
	input.FileHandle = CreateFileW(FileName, FILE_WRITE_DATA, ShareMode, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	ret = (input.FileHandle != INVALID_HANDLE_VALUE) ? ERROR_SUCCESS : GetLastError();
	if (ret == ERROR_SUCCESS)
		ret = _SynchronousIOCTLWriteRequest(IOCTL_REMBUS_DISK_SAVE, &input, sizeof(input));

	CloseHandle(input.FileHandle);

	return ret;
}


DWORD RemBusConnect(ERemBusConnectionMode Mode, PREM_DRIVERS_INFO DriversInfo)
{
	DWORD shareMode = 0;
	DWORD accessMask = 0;
	DWORD ret = ERROR_GEN_FAILURE;

	ret = ERROR_SUCCESS;
	switch (Mode) {
		case rbcmReadOnly:
			accessMask = GENERIC_READ;
			shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
			break;
		case rbcmReadWrite:
			accessMask = GENERIC_READ | GENERIC_WRITE;
			shareMode = FILE_SHARE_READ;
			break;
		default:
			ret = ERROR_NOT_SUPPORTED;
			break;
	}

	if (ret == ERROR_SUCCESS) {
		_deviceHandle = CreateFileW(REMBUS_USER_DEVICE_NAME, accessMask, shareMode, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		ret = (_deviceHandle != INVALID_HANDLE_VALUE) ? ERROR_SUCCESS : GetLastError();
		if (ret == ERROR_SUCCESS) {
			_remDiskHeap = HeapCreate(0, 0, 0);
			ret = (_remDiskHeap != NULL) ? ERROR_SUCCESS : GetLastError();
			if (ret == ERROR_SUCCESS) {
				ret = _SynchronousIOCTLReadRequest(IOCTL_REMBUS_INFO, DriversInfo, sizeof(IOCTL_REMBUS_INFO_OUTPUT));
				if (ret != ERROR_SUCCESS) {
					HeapDestroy(_remDiskHeap);
					_remDiskHeap = NULL;
				}
			}
			
			if (ret != ERROR_SUCCESS) {
				CloseHandle(_deviceHandle);
				_deviceHandle = INVALID_HANDLE_VALUE;
			}
		}
	}

	return ret;
}


VOID RemBusDisconnect(VOID)
{
	HeapDestroy(_remDiskHeap);
	_remDiskHeap = NULL;
	CloseHandle(_deviceHandle);
	_deviceHandle = INVALID_HANDLE_VALUE;


	return;
}
