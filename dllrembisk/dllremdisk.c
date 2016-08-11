
#include <windows.h>
#include "general-types.h"
#include "libremdisk.h"
#include "dllremdisk.h"

/************************************************************************/
/*                    EXPORTED FUNCTIONS                                */
/************************************************************************/

REMDISKAPI DWORD WINAPI DllRemDiskInstall(const WCHAR *BusDriverINFPath)
{
	return RemDiskInstall(BusDriverINFPath);
}


REMDISKAPI DWORD WINAPI DllRemDiskCreate(ULONG DiskNumber, EREMDiskType Type, ULONG64 DiskSize, const wchar_t *FileName, ULONG ShareMode, ULONG Flags, const void *Password, size_t PasswordLength)
{
	return RemDiskCreate(DiskNumber, Type, DiskSize, FileName, ShareMode, Flags, Password, PasswordLength);
}


REMDISKAPI DWORD WINAPI DllRemDiskOpen(ULONG DiskNumber, EREMDiskType Type, const wchar_t *FileName, DWORD ShareMode, ULONG Flags, const unsigned char *Password, size_t PasswordLength)
{
	return RemDiskOpen(DiskNumber, Type, FileName, ShareMode, Flags, Password, PasswordLength);
}


REMDISKAPI DWORD WINAPI DllRemDiskChangePassword(ULONG DiskNumber, const void *NewPassword, size_t NewPasswordLength, const void *OldPassword, size_t OldPasswordLength)
{
	return RemDiskChangePassword(DiskNumber, NewPassword, NewPasswordLength, OldPassword, OldPasswordLength);
}

REMDISKAPI DWORD WINAPI DllRemDiskEncrypt(ULONG DiskNumber, const void *Password, size_t PasswordLength, BOOLEAN Footer)
{
	return RemDiskEncrypt(DiskNumber, Password, PasswordLength, Footer);
}

REMDISKAPI DWORD WINAPI DllRemDiskDecrypt(ULONG DiskNumber, const void *Password, size_t PasswordLength)
{
	return RemDiskDecrypt(DiskNumber, Password, PasswordLength);
}

REMDISKAPI DWORD WINAPI DllRemDiskFree(ULONG DiskNumber)
{
	return RemDiskFree(DiskNumber);
}


REMDISKAPI DWORD WINAPI DllRemDiskEnumerate(PREMDISK_INFO *Array, PULONG Count)
{
	return RemDiskEnumerate(Array, Count);
}


REMDISKAPI VOID WINAPI DllRemDiskEnumerationFree(PREMDISK_INFO Array, ULONG Count)
{
	RemDiskEnumerationFree(Array, Count);

	return;
}


REMDISKAPI DWORD WINAPI DllRemDiskGetInfo(ULONG DiskNumber, PREMDISK_INFO Info)
{
	return RemDiskGetInfo(DiskNumber, Info);
}


REMDISKAPI VOID WINAPI DllRemDiskInfoFree(PREMDISK_INFO Info)
{
	RemDiskInfoFree(Info);

	return;
}


REMDISKAPI DWORD WINAPI DllRemDiskLoadFile(ULONG DiskNumber, const wchar_t *FileName, ULONG ShareMode)
{
	return RemDiskLoadFile(DiskNumber, FileName, ShareMode);
}


REMDISKAPI DWORD WINAPI DllRemDiskSaveFile(ULONG DiskNumber, const wchar_t *FileName, ULONG ShareMode)
{
	return RemDiskSaveFile(DiskNumber, FileName, ShareMode);
}


REMDISKAPI DWORD WINAPI DllRemBusConnect(ERemBusConnectionMode Mode, PREM_DRIVERS_INFO DriversInfo)
{
	return RemBusConnect(Mode, DriversInfo);
}


REMDISKAPI VOID WINAPI DllRemBusDisconnect(VOID)
{
	RemBusDisconnect();

	return;
}



/************************************************************************/
/*                    DLL MAIN                                          */
/************************************************************************/

BOOL WINAPI DllMain(_In_ HINSTANCE hinstDLL, _In_ DWORD     fdwReason, _In_ LPVOID    lpvReserved)
{
	BOOL ret = FALSE;

	switch (fdwReason) {
		case DLL_PROCESS_ATTACH:
			ret = DisableThreadLibraryCalls(hinstDLL);
			break;
	}

	return ret;
}