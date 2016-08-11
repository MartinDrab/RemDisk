
#ifndef __REMXXX_UTILS_H__
#define __REMXXX_UTILS_H__


#include <ntifs.h>
#include "aes.h"
#include "sha2.h"


#define UTILS_ALIGN_UP(Length, Value)			((((Length) + (Value) - 1) / (Value)) * (Value))


#pragma pack(1)

typedef struct _MBR_PARTITION {
	UCHAR Flags;
	UCHAR StartCHS1;
	UCHAR StartCHS2;
	UCHAR StartCHS3;
	UCHAR Type;
	UCHAR EndCHS1;
	UCHAR EndCHS2;
	UCHAR EndCHS3;
	ULONG LBAStart;
	ULONG LBAEnd;
} MBR_PARTITION, *PMBR_PARTITION;

typedef struct _MASTER_BOOT_RECORD {
	UCHAR BootCode[440];
	ULONG DiskId;
	UCHAR Zeroes[2];
	MBR_PARTITION PrimaryPartitions[4];
	UCHAR Byte55h;
	UCHAR ByteAAh;
} MASTER_BOOT_RECORD, *PMASTER_BOOT_RECORD;

#define REMDISK_ENC_FOOTER_SIGNATURE			"REMDISK "


typedef struct _REMDISK_ENCRYPTED_FOOTER {
	UCHAR Signature[8];
	UCHAR DiskKey[16];
	UCHAR Data[4096 - 88];
	UCHAR HMAC[512 / 8];
} REMDISK_ENCRYPTED_FOOTER, *PREMDISK_ENCRYPTED_FOOTER;

#pragma pack()

typedef struct _WDF_WORKITEM_CONTEXT {
	WDFREQUEST Request;
	NTSTATUS Status;
	ULONG_PTR Information;
} WDF_WORKITEM_CONTEXT, *PWDF_WORKITEM_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WDF_WORKITEM_CONTEXT, WorkItemGetContext)


VOID EncryptedFooterCreate(PREMDISK_ENCRYPTED_FOOTER Footer, UCHAR Password[128], UCHAR Key[16]);
NTSTATUS EncryptedFooterLoad(PREMDISK_ENCRYPTED_FOOTER Footer, UCHAR Password[128], UCHAR Key[16]);
NTSTATUS EncryptedFooterChangePassword(PREMDISK_ENCRYPTED_FOOTER Footer, UCHAR NewPassword[128], UCHAR OldPassword[128], UCHAR NewKey[16]);

VOID DeriveKey(const unsigned char *Password, size_t PasswordLength, PUCHAR Key);
VOID XEXEncrypt(const unsigned char *Key, size_t SectorSize, ULONG64 ByteOffset, ULONG64 Length, PUCHAR Data);
VOID XEXDecrypt(const unsigned char *Key, size_t SectorSize, ULONG64 ByteOffset, ULONG64 Length, PUCHAR Data);

NTSTATUS UtilsQCCreate(WDFREQUEST Request, WDFWORKITEM *WorkItem);
VOID UtilsQcRun(WDFWORKITEM WorkItem, NTSTATUS Status, ULONG_PTR Information);



#endif
