
#ifndef __GENERAL_TYPES_H__
#define __GENERAL_TYPES_H__


#define REMBUS_USER_DEVICE_NAME							L"\\\\.\\RemdiskBus"

#define REMDISK_FLAG_WRITABLE							0x1
#define REMDISK_FLAG_COPY_ON_WRITE						0x2
#define REMDISK_FLAG_ALLOCATED							0x4
#define REMDISK_FLAG_SAVE_ON_STOP						0x8
#define REMDISK_FLAG_ENCRYPTED							0x10
#define REMDISK_FLAG_OFFLINE							0x20
#define REMDISK_FLAG_ENCRYPTED_FOOTER					0x40
#define REMDISK_FLAG_FILE_SOURCE						0x80
#define REMDISK_FLAG_SPARSE_FILE						0x100

#define REMBUS_FLAG_SUPPORTS_PLAIN_RAM_DISKS			0x1
#define REMBUS_FLAG_SUPPORTS_PLAIN_FILE_DISKS			0x2
#define REMBUS_FLAG_SUPPORTS_ENCRYPTED_RAM_DISKS		0x4
#define REMBUS_FLAG_SUPPORTS_ENCRYPTED_FILE_DISKS		0x8
#define REMBUS_FLAG_SUPPORTS_EF_RAM_DISKS				0x10
#define REMBUS_FLAG_SUPPORTS_EF_FILE_DISKS				0x20

typedef struct _REM_DRIVERS_INFO {
	ULONG Flags;
	ULONG BusMajorVersion;
	ULONG BusMinorVersion;
	ULONG DiskMajorVersion;
	ULONG DiskMinorVersion;
} REM_DRIVERS_INFO, *PREM_DRIVERS_INFO;

typedef enum _ERemBusConnectionMode {
	rbcmReadOnly,
	rbcmReadWrite,
} ERemBusConnectionMode, *PERemBusConnectionMode;

typedef enum _EREmDiskInfoState {
	rdisInitialized,
	rdisWorking,
	rdisEncrypting,
	rdisDecrypting,
	rdisLoading,
	rdisSaving,
	rdisPasswordChange,
	rdisRemoved,
	rdisSurpriseRemoved,
} EREmDiskInfoState, *PEREmDiskInfoState;

typedef enum _EREMDiskType {
	rdtRAMDisk,
	rdtFileDisk,
} EREMDiskType, *PEREMDiskType;


typedef struct _REMDISK_INFO {
	ULONG DiskNumber;
	EREMDiskType Type;
	ULONG64 DiskSize;
	ULONG Flags;
	EREmDiskInfoState State;
	PWCHAR FileName;
} REMDISK_INFO, *PREMDISK_INFO;

typedef enum _EREMDiskCryptoOperationType {
	rdcotSetPassword,
	rdcotClearPassword,
	rdcotEncrypt,
	rdcotDecrypt,
} EREMDiskCryptoOperationType, *PEREMDiskCryptoOperationType;



#endif
