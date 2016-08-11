
#include <ntifs.h>
#include <wdf.h>
#include "preprocessor.h"
#include "wdf-symbolic-link.h"


/************************************************************************/
/*                  HELPER FUNCTIONS                                    */
/************************************************************************/

static void _WdfSymbolicLinkEvtCleanup(WDFSYMBOLICLINK Link)
{
	PWDF_SYMBOLIC_LINK_CONTEXT ctx = WdfSymbolicLinkGetcontext(Link);
	DEBUG_ENTER_FUNCTION("Link=0x%p", Link);

	if (ctx->Initialized)
		IoDeleteSymbolicLink(&ctx->LinkName);

	if (ctx->LinkName.Buffer != NULL)
		ExFreePoolWithTag(ctx->LinkName.Buffer, ctx->PoolTag);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

/************************************************************************/
/*                  PUBLIC FUNCTIONS                                    */
/************************************************************************/


NTSTATUS WdfSymbolicLinkCreate(WDFOBJECT Parent, PUNICODE_STRING Source, PUNICODE_STRING Target, ULONG PoolTag, WDFSYMBOLICLINK *Link)
{
	WDFSYMBOLICLINK tmpLink = NULL;
	WDF_OBJECT_ATTRIBUTES attributes;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Parent=0x%p; Source=\"%wZ\"; Target=\"%wZ\"; Link=0x%p", Parent, Source, Target, PoolTag, Link);

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, WDF_SYMBOLIC_LINK_CONTEXT);
	attributes.EvtCleanupCallback = _WdfSymbolicLinkEvtCleanup;
	if (Parent != NULL)
		attributes.ParentObject = Parent;

	status = WdfObjectCreate(&attributes, &tmpLink);
	if (NT_SUCCESS(status)) {
		PWDF_SYMBOLIC_LINK_CONTEXT ctx = WdfSymbolicLinkGetcontext(tmpLink);

		ctx->Initialized = FALSE;
		ctx->PoolTag = PoolTag;
		ctx->LinkName = *Source;
		ctx->LinkName.MaximumLength = ctx->LinkName.Length;
		ctx->LinkName.Buffer = (PWCH)ExAllocatePoolWithTag(PagedPool, ctx->LinkName.Length, ctx->PoolTag);
		if (ctx->LinkName.Buffer != NULL) {
			memcpy(ctx->LinkName.Buffer, Source->Buffer, ctx->LinkName.Length);
			status = IoCreateSymbolicLink(&ctx->LinkName, Target);
			ctx->Initialized = NT_SUCCESS(status);
			if (NT_SUCCESS(status))
				*Link = tmpLink;
		} else status = STATUS_INSUFFICIENT_RESOURCES;

		if (!NT_SUCCESS(status))
			WdfObjectDelete(tmpLink);
	}

	DEBUG_EXIT_FUNCTION("0x%x, *Link=0x%p", status, *Link);
	return status;
}
