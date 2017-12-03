
#include <ntifs.h>
#include <wdf.h>
#include "preprocessor.h"
#include "aes.h"
#include "sha2.h"
#include "hmac_sha2.h"
#include "utils.h"



/************************************************************************/
/*                   HELPER FUNCTIONS                                   */
/************************************************************************/

static void _XOR128(PUCHAR Dest, const UCHAR *Source)
{
	*(PULONG64)Dest ^= *(PULONG64)Source;
	*((PULONG64)Dest + 1) ^= *((PULONG64)Source + 1);

	return;
}


static VOID _Multiply128(PUCHAR Dest, const ULONG64 Value)
{
	ULONG32 A[4];
	ULONG32 B[4];
	PULONG32 result = (PULONG32)Dest;
	ULONG32 tmp[4];

	RtlSecureZeroMemory(B, sizeof(B));
	RtlCopyMemory(A, Dest, sizeof(A));
	RtlCopyMemory(B, &Value, sizeof(Value));
	RtlSecureZeroMemory(result, sizeof(A));
	for (size_t j = 0; j < 4; ++j) {
		ULONG32 remainder = 0;
		RtlSecureZeroMemory(tmp, sizeof(tmp));
		for (size_t i = 0; i < 4 - j; ++i) {
			ULARGE_INTEGER x;

			x.QuadPart = (ULONG64)B[j] * (ULONG64)A[i] + (ULONG64)remainder;
			remainder = x.HighPart;
			tmp[j + i] = x.LowPart;
		}

		remainder = 0;
		for (size_t i = 0; i < 4; ++i) {
			ULARGE_INTEGER x;

			x.QuadPart = (ULONG64)result[i] + (ULONG64)tmp[i] + (ULONG64)remainder;
			remainder = x.HighPart;
			result[i] = x.LowPart;
		}
	}

	return;
}


static VOID _EtwQcWorkItem(WDFWORKITEM WorkItem)
{
	PWDF_WORKITEM_CONTEXT ctx = WorkItemGetContext(WorkItem);
	DEBUG_ENTER_FUNCTION("0x%p", WorkItem);

	WdfRequestCompleteWithInformation(ctx->Request, ctx->Status, ctx->Information);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

/************************************************************************/
/*                    PUBLIC FUNCTIONS                                  */
/************************************************************************/


VOID XEXEncrypt(const unsigned char *Key, size_t SectorSize, ULONG64 ByteOffset, ULONG64 Length, PUCHAR Data)
{
	UCHAR ct[16];
	unsigned long rk[RKLENGTH(128)];
	unsigned int nr = NROUNDS(128);
	DEBUG_ENTER_FUNCTION("Key=0x%p; SectorSize=%Iu; ByteOffset=%I64u; Length=%I64u; Data=0x%p", Key, SectorSize, ByteOffset, Length, Data);

	nr = rijndaelSetupEncrypt(rk, Key, 128);
	for (ULONG64 i = 0; i < Length / SectorSize; ++i) {
		unsigned char sectorIndex[16];

		*(PULONG64)sectorIndex = (ByteOffset / SectorSize) + i;
		*((PULONG64)sectorIndex + 1) = 0;
		rijndaelEncrypt(rk, nr, sectorIndex, ct);
		RtlCopyMemory(sectorIndex, ct, sizeof(sectorIndex));

		ULONG64 jValue = 1;
		for (ULONG64 j = 0; j < SectorSize / sizeof(sectorIndex); ++j) {
			const PUCHAR pt = Data + i*SectorSize + j*sizeof(sectorIndex);
			unsigned char X[sizeof(sectorIndex)];

			RtlCopyMemory(X, sectorIndex, sizeof(sectorIndex));
			_Multiply128(X, jValue);
			_XOR128(pt, X);
			rijndaelEncrypt(rk, nr, pt, ct);
			_XOR128(ct, X);
			memcpy(pt, ct, sizeof(sectorIndex));
			jValue <<= 1;
		}
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


VOID XEXDecrypt(const unsigned char *Key, size_t SectorSize, ULONG64 ByteOffset, ULONG64 Length, PUCHAR Data)
{
	UCHAR pt[16];
	unsigned long rk[RKLENGTH(128)];
	unsigned long rkDec[RKLENGTH(128)];
	unsigned int nr = NROUNDS(128);
	unsigned int nrDec = NROUNDS(128);
	DEBUG_ENTER_FUNCTION("Key=0x%p; SectorSize=%Iu; ByteOffset=%I64u; Length=%I64u; Data=0x%p", Key, SectorSize, ByteOffset, Length, Data);

	nr = rijndaelSetupEncrypt(rk, Key, 128);
	nrDec = rijndaelSetupDecrypt(rkDec, Key, 128);
	for (ULONG64 i = 0; i < Length / SectorSize; ++i) {
		unsigned char sectorIndex[16];

		*(PULONG64)sectorIndex = (ByteOffset / SectorSize) + i;
		*((PULONG64)sectorIndex + 1) = 0;
		rijndaelEncrypt(rk, nr, sectorIndex, pt);
		RtlCopyMemory(sectorIndex, pt, sizeof(sectorIndex));

		ULONG64 jValue = 1;
		for (ULONG64 j = 0; j < SectorSize / sizeof(sectorIndex); ++j) {
			const PUCHAR ct = Data + i*SectorSize + j*sizeof(sectorIndex);
			unsigned char X[sizeof(sectorIndex)];

			RtlCopyMemory(X, sectorIndex, sizeof(sectorIndex));
			_Multiply128(X, jValue);
			_XOR128(ct, X);
			rijndaelDecrypt(rkDec, nrDec, ct, pt);
			_XOR128(pt, X);
			memcpy(ct, pt, sizeof(sectorIndex));
			jValue <<= 1;
		}
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


VOID DeriveKey(const unsigned char *Password, size_t PasswordLength, PUCHAR Key)
{
	UCHAR digest[SHA512_DIGEST_SIZE];
	UCHAR msg[128 + SHA512_DIGEST_SIZE];
	DEBUG_ENTER_FUNCTION("Password=0x%p; PasswordLength=%Iu; Key=0x%p", Password, PasswordLength, Key);

	ASSERT(PasswordLength == 128);
	RtlCopyMemory(msg, Password, PasswordLength);
	sha512(msg, (ULONG)PasswordLength, digest);
	RtlCopyMemory(msg + PasswordLength, digest, sizeof(digest));
	for (size_t i = 0; i < 1024; ++i) {
		sha512(msg, sizeof(msg), digest);
		RtlCopyMemory(msg + PasswordLength, digest, sizeof(digest));
	}

	RtlCopyMemory(Key, digest, 16);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


NTSTATUS UtilsQCCreate(WDFREQUEST Request, WDFWORKITEM *WorkItem)
{
	WDFWORKITEM workItem = NULL;
	PWDF_WORKITEM_CONTEXT ctx = NULL;
	WDF_OBJECT_ATTRIBUTES attrs;
	WDF_WORKITEM_CONFIG config;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Request=0x%p; WorkItem=0x%p", Request, WorkItem);

	WDF_WORKITEM_CONFIG_INIT(&config, _EtwQcWorkItem);
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attrs, WDF_WORKITEM_CONTEXT);
	attrs.ParentObject = Request;
	status = WdfWorkItemCreate(&config, &attrs, &workItem);
	if (NT_SUCCESS(status)) {
		ctx = WorkItemGetContext(workItem);
		ctx->Request = Request;
		*WorkItem = workItem;
	}

	DEBUG_EXIT_FUNCTION("0x%x, *WorkItem=0x%p", status, *WorkItem);
	return status;
}


VOID UtilsQcRun(WDFWORKITEM WorkItem, NTSTATUS Status, ULONG_PTR Information)
{
	PWDF_WORKITEM_CONTEXT ctx = NULL;

	ctx = WorkItemGetContext(WorkItem);
	ctx->Status = Status;
	ctx->Information = Information;
	WdfWorkItemEnqueue(WorkItem);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
