
#ifndef __WDF_RWLOCK_H__
#define __WDF_RWLOCK_H__

#include <ntifs.h>
#include <wdf.h>


typedef WDFOBJECT WDFRWLOCK;

typedef struct _WDF_RWLOCK_CONTEXT{
	ERESOURCE Lock;
	BOOLEAN Initialized;
} WDF_RWLOCK_CONTEXT, *PWDF_RWLOCK_CONTEXT;


WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WDF_RWLOCK_CONTEXT, WdfLockGetContext)


NTSTATUS WdfRWLockCreate(WDFOBJECT Parent, WDFRWLOCK *Lock);
VOID WdfRWLockAcquireShared(WDFRWLOCK Lock);
VOID WdfRWLockAcquireExclusive(WDFRWLOCK Lock);
VOID WdfRWLockRelease(WDFRWLOCK Lock);



#endif 
