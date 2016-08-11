
#ifndef __DLLREMDISK_H__
#define __DLLREMDISK_H__

#include <windows.h>
#include "general-types.h"



#ifdef DLLREMBISK_EXPORTS

#define REMDISKAPI						EXTERN_C __declspec(dllexport)

#else

#define REMDISKAPI						EXTERN_C __declspec(dllimport)

#endif


REMDISKAPI DWORD WINAPI DllRemDiskInstall(const WCHAR *BusDriverINFPath);
REMDISKAPI DWORD WINAPI DllRemDiskCreate(ULONG DiskNumber, EREMDiskType Type, ULONG64 DiskSize, const wchar_t *FileName, ULONG ShareMode, ULONG Flags, const void *Password, size_t PasswordLength);
REMDISKAPI DWORD WINAPI DllRemDiskOpen(ULONG DiskNumber, EREMDiskType Type, const wchar_t *FileName, DWORD ShareMode, ULONG Flags, const unsigned char *Password, size_t PasswordLength);
REMDISKAPI DWORD WINAPI DllRemDiskChangePassword(ULONG DiskNumber, const void *NewPassword, size_t NewPasswordLength, const void *OldPassword, size_t OldPasswordLength);
REMDISKAPI DWORD WINAPI DllRemDiskEncrypt(ULONG DiskNumber, const void *Password, size_t PasswordLength, BOOLEAN Footer);
REMDISKAPI DWORD WINAPI DllRemDiskDecrypt(ULONG DiskNumber, const void *Password, size_t PasswordLength);
REMDISKAPI DWORD WINAPI DllRemDiskFree(ULONG DiskNumber);
REMDISKAPI DWORD WINAPI DllRemDiskEnumerate(PREMDISK_INFO *Array, PULONG Count);
REMDISKAPI VOID WINAPI DllRemDiskEnumerationFree(PREMDISK_INFO Array, ULONG Count);
REMDISKAPI DWORD WINAPI DllRemDiskGetInfo(ULONG DiskNumber, PREMDISK_INFO Info);
REMDISKAPI VOID WINAPI DllRemDiskInfoFree(PREMDISK_INFO Info);
REMDISKAPI DWORD WINAPI DllRemDiskLoadFile(ULONG DiskNumber, const wchar_t *FileName, ULONG ShareMode);
REMDISKAPI DWORD WINAPI DllRemDiskSaveFile(ULONG DiskNumber, const wchar_t *FileName, ULONG ShareMode);

REMDISKAPI DWORD WINAPI DllRemBusConnect(ERemBusConnectionMode Mode, PREM_DRIVERS_INFO DriversInfo);
REMDISKAPI VOID WINAPI DllRemBusDisconnect(VOID);








#endif
