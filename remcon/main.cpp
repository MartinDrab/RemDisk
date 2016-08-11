
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "general-types.h"
#include "dllremdisk.h"


// *install <BusInf>
// *ram <DiskNumber> <Size>
// *encram <DiskNumber> <Size> <Password>
// *file <DiskNumber> <File>
// *encfile <DiskNumber> <File> <Password>
// *free <DiskNumber>
// save <DiskNumber> <FileName>
// load <DiskNumber> <FileName>
// *query <DiskNumber>
// enum



static void _PrintDiskInfo(const REMDISK_INFO & DiskInfo)
{
	const wchar_t *typeArray[] = {
		L"RAM disk",
		L"File disk"
	};
	
	const wchar_t *stateArray[] = {
		L"Initialized",
		L"Working",
		L"Encrypting",
		L"Decrypting",
		L"Loading",
		L"Saving",
		L"PasswordChange",
		L"Removed",
		L"SurpriseRemoval",
	};

	printf("  Disk number:        %u\n", DiskInfo.DiskNumber);
	printf("  Disk size:          %I64u B (%I64u MB)\n", DiskInfo.DiskSize, DiskInfo.DiskSize / (1024*1024));
	printf("  Type:               %S (%u)\n", typeArray[DiskInfo.Type], DiskInfo.Type);
	printf("  State:              %S (%u)\n", stateArray[DiskInfo.State], DiskInfo.State);
	printf("  Writable:           %u\n", (DiskInfo.Flags & REMDISK_FLAG_WRITABLE) != 0);
	printf("  Allocated:          %u\n", (DiskInfo.Flags & REMDISK_FLAG_ALLOCATED) != 0);
	printf("  Encrypted:          %u\n", (DiskInfo.Flags & REMDISK_FLAG_ENCRYPTED) != 0);
	printf("  Offline:            %u\n", (DiskInfo.Flags & REMDISK_FLAG_OFFLINE) != 0);
	printf("  File name           %S\n", DiskInfo.FileName);

	return;
}


int wmain(int argc, wchar_t **argv)
{
	bool connected = false;
	ULONG diskNumber = 0;
	ULONG diskSize = 0;
	wchar_t *fileName = NULL;
	DWORD err = ERROR_GEN_FAILURE;
	REM_DRIVERS_INFO driversInfo;

	if (argc != 3 || wcsicmp(argv[1], L"install") != 0) {
		printf("Connecting to the driver (RW)...");
		err = DllRemBusConnect(rbcmReadWrite, &driversInfo);
		if (err != ERROR_SUCCESS) {
			printf("ERROR %u\n", err);
			printf("Connecting to the driver (RO)...");
			err = DllRemBusConnect(rbcmReadOnly, &driversInfo);
			if (err != ERROR_SUCCESS)
				printf("ERROR %u\n", err);
		}

		connected = (err == ERROR_SUCCESS);
	} else err = ERROR_SUCCESS;

	if (err == ERROR_SUCCESS) {
		printf("OK\n");
		switch (argc) {
		case 2:
			if (wcsicmp(L"enum", argv[1]) == 0) {
				ULONG count = 0;
				PREMDISK_INFO diskInfo = NULL;

				err = DllRemDiskEnumerate(&diskInfo, &count);
				if (err == ERROR_SUCCESS) {
					printf("All disk have been enumerated successfully\n");
					for (size_t i = 0; i < count; ++i) {
						_PrintDiskInfo(diskInfo[i]);
						printf("\n");
					}

					DllRemDiskEnumerationFree(diskInfo, count);
				} else printf("ERROR: Failed to enumerate disks: %u\n", err);
			} else printf("ERROR: Invalid command specified (%S)\n", argv[1]);
			break;
		case 3:
			diskNumber = wcstoul(argv[2], NULL, 0);
			if (wcsicmp(L"free", argv[1]) == 0) {
				if (diskNumber > 0) {
					err = DllRemDiskFree(diskNumber);
					if (err == ERROR_SUCCESS)
						printf("The disk has been successfully removed\n");
					else printf("ERROR: Failed to remove the disk: %u\n", err);
				} else printf("ERROR: Invalid disk number specified (%u)\n", diskNumber);
			} else if (wcsicmp(L"query", argv[1]) == 0) {
				if (diskNumber > 0) {
					REMDISK_INFO diskInfo;

					err = DllRemDiskGetInfo(diskNumber, &diskInfo);
					if (err == ERROR_SUCCESS) {
						printf("The disk information has been successfully queried\n");
						_PrintDiskInfo(diskInfo);
						DllRemDiskInfoFree(&diskInfo);
					} else printf("ERROR: Failed to query the disk iformation: %u\n", err);
				} else printf("ERROR: Invalid disk number specified (%u)\n", diskNumber);
			} else if (wcsicmp(L"install", argv[1]) == 0) {
				const wchar_t *busInf = argv[2];

				err = DllRemDiskInstall(busInf);
				if (err == ERROR_SUCCESS)
					printf("Drivers installed successfully\n");
				else printf("ERROR: Failed to install drivers: %u\n", err);
			} else printf("ERROR: Invalid command specified (%S)\n", argv[1]);
			break;
		case 4:
			if (wcsicmp(L"encrypt", argv[1]) == 0) {
				if (diskNumber > 0) {
					const wchar_t *password = argv[3];

					err = DllRemDiskEncrypt(diskNumber, password, wcslen(password)*sizeof(wchar_t), FALSE);
					if (err == ERROR_SUCCESS)
						printf("The disk disk has been successfully encrypted\n");
					else printf("ERROR: Failed to encrypt the disk: %u\n", err);
				} else printf("ERROR: Invalid disk number specified (%u)\n", diskNumber);
			} else if (wcsicmp(L"decrypt", argv[1]) == 0) {
				if (diskNumber > 0) {
					const wchar_t *password = argv[3];

					err = DllRemDiskDecrypt(diskNumber, password, wcslen(password)*sizeof(wchar_t));
					if (err == ERROR_SUCCESS)
						printf("The disk disk has been successfully decrypted\n");
					else printf("ERROR: Failed to decrypted the disk: %u\n", err);
				} else printf("ERROR: Invalid disk number specified (%u)\n", diskNumber);
			} else if (wcsicmp(L"ram", argv[1]) == 0) {
				diskNumber = wcstoul(argv[2], NULL, 0);
				if (diskNumber > 0) {
					diskSize = wcstoul(argv[3], NULL, 0);
					if (diskSize > 64) {
						ULONG64 diskSize64 = diskSize * 1024 * 1024;

						err = DllRemDiskCreate(diskNumber, rdtRAMDisk, diskSize64, NULL, FILE_SHARE_READ, REMDISK_FLAG_WRITABLE, NULL, 0);
						if (err == ERROR_SUCCESS)
							printf("The disk has been successfully created\n");
						else printf("ERROR: Disk creation failed with error %u\n", err);
					} else printf("ERROR: Invalid disk size specified (%u)\n", diskNumber);
				} else printf("ERROR: Invalid disk number specified (%u)\n", diskNumber);
			} else if (wcsicmp(L"save", argv[1]) == 0) {
				diskNumber = wcstoul(argv[2], NULL, 0);
				if (diskNumber > 0) {
					fileName = argv[3];
					err = DllRemDiskSaveFile(diskNumber, fileName, 0);
					if (err == ERROR_SUCCESS)
						printf("The disk image has been saved to the file\n");
					else printf("ERROR: Failed to save the disk image: %u\n", err);
				} else printf("ERROR: Invalid disk number specified (%u)\n", diskNumber);
			}  else if (wcsicmp(L"file", argv[1]) == 0) {
				diskNumber = wcstoul(argv[2], NULL, 0);
				if (diskNumber > 0) {
					fileName = argv[3];
					err = DllRemDiskCreate(diskNumber, rdtFileDisk, 0, fileName, FILE_SHARE_READ, REMDISK_FLAG_WRITABLE, NULL, 0);
					if (err == ERROR_SUCCESS)
						printf("The disk has been successfully created\n");
					else printf("ERROR: Disk creation failed with error %u\n", err);
				} else printf("ERROR: Invalid disk number specified (%u)\n", diskNumber);
			} else printf("ERROR: Invalid command specified (%S)\n", argv[1]);
			break;
		case 5:
			if (wcsicmp(L"ram", argv[1]) == 0) {
				diskNumber = wcstoul(argv[2], NULL, 0);
				if (diskNumber > 0) {
					diskSize = wcstoul(argv[3], NULL, 0);
					if (diskSize > 64) {
						ULONG64 diskSize64 = diskSize * 1024 * 1024;

						fileName = argv[4];
						err = DllRemDiskCreate(diskNumber, rdtRAMDisk, diskSize64, fileName, FILE_SHARE_READ, REMDISK_FLAG_WRITABLE, NULL, 0);
						if (err == ERROR_SUCCESS)
							printf("The disk has been successfully created\n");
						else printf("ERROR: Disk creation failed with error %u\n", err);
					} else printf("ERROR: Invalid disk size specified (%u)\n", diskNumber);
				} else printf("ERROR: Invalid disk number specified (%u)\n", diskNumber);
			} else if (wcsicmp(L"encram", argv[1]) == 0) {
				diskNumber = wcstoul(argv[2], NULL, 0);
				if (diskNumber > 0) {
					diskSize = wcstoul(argv[3], NULL, 0);
					if (diskSize > 64) {
						ULONG64 diskSize64 = diskSize * 1024 * 1024;
						const wchar_t *password = argv[4];

						err = DllRemDiskCreate(diskNumber, rdtRAMDisk, diskSize64, NULL, FILE_SHARE_READ, REMDISK_FLAG_WRITABLE | REMDISK_FLAG_ENCRYPTED, password, wcslen(password)*sizeof(wchar_t));
						if (err == ERROR_SUCCESS)
							printf("The disk has been successfully created\n");
						else printf("ERROR: Disk creation failed with error %u\n", err);
					}
					else printf("ERROR: Invalid disk size specified (%u)\n", diskNumber);
				}
				else printf("ERROR: Invalid disk number specified (%u)\n", diskNumber);
			} else  if (wcsicmp(L"encfile", argv[1]) == 0) {
					diskNumber = wcstoul(argv[2], NULL, 0);
					if (diskNumber > 0) {
						wchar_t *password = argv[4];

						fileName = argv[3];
						err = DllRemDiskCreate(diskNumber, rdtFileDisk, 0, fileName, FILE_SHARE_READ, REMDISK_FLAG_WRITABLE | REMDISK_FLAG_ENCRYPTED, password, wcslen(password)*sizeof(wchar_t));
						if (err == ERROR_SUCCESS)
							printf("The disk has been successfully created\n");
						else printf("ERROR: Disk creation failed with error %u\n", err);
					} else printf("ERROR: Invalid disk number specified (%u)\n", diskNumber);
			} else if (wcsicmp(L"password", argv[1]) == 0) {
				diskNumber = wcstoul(argv[2], NULL, 0);
				if (diskNumber > 0) {
					const wchar_t *password = argv[3];
					const wchar_t *oldPassword = argv[4];

					err = DllRemDiskChangePassword(diskNumber, password, wcslen(password)*sizeof(wchar_t), oldPassword, wcslen(oldPassword)*sizeof(wchar_t));
					if (err == ERROR_SUCCESS)
						printf("Password successfully set\n");
					else printf("ERROR: Failed to set the password for the disk %u\n", err);
				} else printf("ERROR: Invalid disk number specified (%u)\n", diskNumber);
			} else printf("ERROR: Invalid command specified (%S)\n", argv[1]);
			break;
		case 6:
			if (wcsicmp(L"encram", argv[1]) == 0) {
				diskNumber = wcstoul(argv[2], NULL, 0);
				if (diskNumber > 0) {
					diskSize = wcstoul(argv[3], NULL, 0);
					if (diskSize > 64) {
						ULONG64 diskSize64 = diskSize * 1024 * 1024;
						const wchar_t *password = argv[5];

						fileName = argv[4];
						err = DllRemDiskCreate(diskNumber, rdtRAMDisk, diskSize64, fileName, FILE_SHARE_READ, REMDISK_FLAG_WRITABLE | REMDISK_FLAG_ENCRYPTED, password, wcslen(password)*sizeof(wchar_t));
						if (err == ERROR_SUCCESS)
							printf("The disk has been successfully created\n");
						else printf("ERROR: Disk creation failed with error %u\n", err);
					} else printf("ERROR: Invalid disk size specified (%u)\n", diskNumber);
				} else printf("ERROR: Invalid disk number specified (%u)\n", diskNumber);
			} else printf("ERROR: Invalid command (%s)\n", argv[1]);
			break;
		default:
			err = ERROR_INVALID_PARAMETER;
			break;
		}
		
		if (connected)
			DllRemBusDisconnect();
	}

	return (int)err;
}