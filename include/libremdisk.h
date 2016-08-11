
#ifndef __LIBREMDISK_H__
#define __LIBREMDISK_H__

#include <windows.h>
#include "general-types.h"



DWORD RemDiskInstall(const WCHAR *BusDriverINFPath);
DWORD RemDiskCreate(ULONG DiskNumber, EREMDiskType Type, ULONG64 DiskSize, const wchar_t *FileName, ULONG ShareMode, ULONG Flags, const void *Password, size_t PasswordLength);
DWORD RemDiskOpen(ULONG DiskNumber, EREMDiskType Type, const wchar_t *FileName, DWORD ShareMode, ULONG Flags, const unsigned char *Password, size_t PasswordLength);
DWORD RemDiskChangePassword(ULONG DiskNumber, const void *NewPassword, size_t NewPasswordLength, const void *OldPassword, size_t OldPasswordLength);
DWORD RemDiskEncrypt(ULONG DiskNumber, const void *Password, size_t PasswordLength, BOOLEAN Footer);
DWORD RemDiskDecrypt(ULONG DiskNumber, const void *Password, size_t PasswordLength);
DWORD RemDiskFree(ULONG DiskNumber);
DWORD RemDiskEnumerate(PREMDISK_INFO *Array, PULONG Count);
VOID RemDiskEnumerationFree(PREMDISK_INFO Array, ULONG Count);
DWORD RemDiskGetInfo(ULONG DiskNumber, PREMDISK_INFO Info);
VOID RemDiskInfoFree(PREMDISK_INFO Info);
DWORD RemDiskLoadFile(ULONG DiskNumber, const wchar_t *FileName, ULONG ShareMode);
DWORD RemDiskSaveFile(ULONG DiskNumber, const wchar_t *FileName, ULONG ShareMode);

DWORD RemBusConnect(ERemBusConnectionMode Mode, PREM_DRIVERS_INFO DriversInfo);
VOID RemBusDisconnect(VOID);



#endif 
