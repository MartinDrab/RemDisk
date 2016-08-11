
#include <ntifs.h>
#include <wdf.h>
#include "preprocessor.h"
#include "wdf-rwlock.h"


/************************************************************************/
/*            HELPER FUNCTIONS                                          */
/************************************************************************/


static void _RWLockEtvCleanup(WDFRWLOCK Lock)
{
	PWDF_RWLOCK_CONTEXT ctx = WdfLockGetContext(Lock);
	DEBUG_ENTER_FUNCTION("Lock=0x%p", Lock);

	if (ctx->Initialized)
		ExDeleteResourceLite(&ctx->Lock);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


/************************************************************************/
/*           PUBLIC FUNCTIONS                                           */
/************************************************************************/


NTSTATUS WdfRWLockCreate(WDFOBJECT Parent, WDFRWLOCK *Lock)
{
	WDFRWLOCK tmpLock = NULL;
	WDF_OBJECT_ATTRIBUTES attributes;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Parent=0x%p; Lock=0x%p", Parent, Lock);

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, WDF_RWLOCK_CONTEXT);
	attributes.EvtCleanupCallback = _RWLockEtvCleanup;
	if (Parent != NULL)
		attributes.ParentObject = Parent;

	status = WdfObjectCreate(&attributes, &tmpLock);
	if (NT_SUCCESS(status)) {
		PWDF_RWLOCK_CONTEXT ctx = WdfLockGetContext(tmpLock);

		status = ExInitializeResourceLite(&ctx->Lock);
		ctx->Initialized = NT_SUCCESS(status);
		if (NT_SUCCESS(status))
			*Lock = tmpLock;

		if (!NT_SUCCESS(status))
			WdfObjectDelete(tmpLock);
	}

	DEBUG_EXIT_FUNCTION("0x%x, *Lock=0x%p", status, *Lock);
	return status;
}


VOID WdfRWLockAcquireShared(WDFRWLOCK Lock)
{
	PWDF_RWLOCK_CONTEXT ctx = WdfLockGetContext(Lock);

	KeEnterCriticalRegion();
	ExAcquireResourceSharedLite(&ctx->Lock, TRUE);

	return;
}


VOID WdfRWLockAcquireExclusive(WDFRWLOCK Lock)
{
	PWDF_RWLOCK_CONTEXT ctx = WdfLockGetContext(Lock);

	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(&ctx->Lock, TRUE);

	return;
}


VOID WdfRWLockRelease(WDFRWLOCK Lock)
{
	PWDF_RWLOCK_CONTEXT ctx = WdfLockGetContext(Lock);

	ExReleaseResourceLite(&ctx->Lock);
	KeLeaveCriticalRegion();

	return;
}

