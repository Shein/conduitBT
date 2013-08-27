/*++

Module Name:
    clisrv.c

Abstract:
    Implementation of functions common to bth HFP server and client

Environment:
    Kernel mode
--*/

#include "driver.h"
#include "device.h"
#include "clisrv.h"

#if defined(EVENT_TRACING)
#include "clisrv.tmh"
#endif


_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS HfpSharedDeviceContextInit (HFPDEVICE_CONTEXT* devCtx, WDFDEVICE Device)
{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;
        
    devCtx->Device   = Device;
    devCtx->IoTarget = WdfDeviceGetIoTarget(Device);

    // Initialize request object
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = Device;

    status = WdfRequestCreate (&attributes, devCtx->IoTarget, &devCtx->Request);

    if (!NT_SUCCESS(status))  {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "Failed to pre-allocate request in device context, Status=%X", status);
        goto exit;        
    }

	exit:
    return status;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS HfpSharedRetrieveLocalInfo (_In_ HFPDEVICE_CONTEXT* devCtx)
{
    NTSTATUS status = STATUS_SUCCESS;
    struct _BRB_GET_LOCAL_BD_ADDR * brb;
    
    brb = (struct _BRB_GET_LOCAL_BD_ADDR*) devCtx->ProfileDrvInterface.BthAllocateBrb (BRB_HCI_GET_LOCAL_BD_ADDR, POOLTAG_HFPDRIVER);
    if (!brb) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "Failed to allocate brb BRB_HCI_GET_LOCAL_BD_ADDR, Status %X", status);        
        goto exit;
    }

    status = HfpSharedSendBrbSynchronously (devCtx->IoTarget, devCtx->Request, (PBRB) brb, sizeof(*brb));
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "Retrieving local bth address failed, Status=%X", status);        
        goto exit1;        
    }

    devCtx->LocalBthAddr = brb->BtAddress;

#if (NTDDI_VERSION >= NTDDI_WIN8)
    // Now retrieve local host supported features
    status = HfpSharedGetHostSupportedFeatures(devCtx);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "Sending IOCTL for reading supported features failed, Status=%X", status);
        goto exit1;
    }
#endif
    
	exit1:
    devCtx->ProfileDrvInterface.BthFreeBrb ((PBRB)brb);

	exit:
    return status;
}


#if (NTDDI_VERSION >= NTDDI_WIN8)

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS HfpSharedGetHostSupportedFeatures(_In_ HFPDEVICE_CONTEXT* devCtx)
{   
    WDF_MEMORY_DESCRIPTOR	outMemDesc = {0};
    BTH_HOST_FEATURE_MASK	localFeatures = {0};
    NTSTATUS				status = STATUS_SUCCESS;

    devCtx->LocalFeatures.Mask = 0;

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER (&outMemDesc, &localFeatures, sizeof(localFeatures));

    status = WdfIoTargetSendIoctlSynchronously(
        devCtx->IoTarget,
        NULL,
        IOCTL_BTH_GET_HOST_SUPPORTED_FEATURES,
        NULL,
        &outMemDesc,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;
    
    devCtx->LocalFeatures = localFeatures;
    return status;
}

#endif


_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS HfpSharedSendBrbAsync(
    _In_ WDFIOTARGET IoTarget,
    _In_ WDFREQUEST Request,
    _In_ PBRB Brb,
    _In_ size_t BrbSize,
    _In_ PFN_WDF_REQUEST_COMPLETION_ROUTINE ComplRoutine,
    _In_opt_ WDFCONTEXT Context
    )
{
    NTSTATUS				status = BTH_ERROR_SUCCESS;
    WDF_OBJECT_ATTRIBUTES	attributes;
    WDFMEMORY				memoryArg1 = NULL;

    if (BrbSize <= 0) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_CONT_READER, "BrbSize has invalid value: %I64d", BrbSize);
        status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = Request;

    status = WdfMemoryCreatePreallocated (&attributes, Brb, BrbSize, &memoryArg1);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_UTIL, "Creating preallocated memory for BRB 0x%p failed, Request to be formatted 0x%p, Status %X", Brb, Request, status);
        goto exit;
    }

    status = WdfIoTargetFormatRequestForInternalIoctlOthers(
        IoTarget,
        Request,
        IOCTL_INTERNAL_BTH_SUBMIT_BRB,
        memoryArg1,
        NULL, //OtherArg1Offset
        NULL, //OtherArg2
        NULL, //OtherArg2Offset
        NULL, //OtherArg4
        NULL  //OtherArg4Offset
        );

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_UTIL, "Formatting request 0x%p with BRB 0x%p failed, Status %X", Request, Brb, status);
        goto exit;
    }

    // Set a CompletionRoutine callback function.
    WdfRequestSetCompletionRoutine (Request, ComplRoutine, Context);

    if (!WdfRequestSend(Request, IoTarget, NULL)) {
        status = WdfRequestGetStatus(Request);
        TraceEvents(TRACE_LEVEL_ERROR, DBG_UTIL, "Request send failed for request 0x%p, BRB 0x%p, Status %X", Request, Brb, status);
        goto exit;
    }

	exit:
    return status;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS FORCEINLINE HfpSharedSendBrbSynchronously (_In_ WDFIOTARGET IoTarget, _In_ WDFREQUEST Request, _In_ PBRB Brb, _In_ ULONG BrbSize)
{
    NTSTATUS					status;
    WDF_REQUEST_REUSE_PARAMS	reuseParams;
    WDF_MEMORY_DESCRIPTOR		OtherArg1Desc;


    WDF_REQUEST_REUSE_PARAMS_INIT (&reuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_NOT_SUPPORTED);

    status = WdfRequestReuse(Request, &reuseParams);
    if (!NT_SUCCESS(status))
        goto exit;

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER (&OtherArg1Desc, Brb, BrbSize);

    status = WdfIoTargetSendInternalIoctlOthersSynchronously(
        IoTarget,
        Request,
        IOCTL_INTERNAL_BTH_SUBMIT_BRB,
        &OtherArg1Desc,
        NULL, //OtherArg2
        NULL, //OtherArg4
        NULL, //RequestOptions
        NULL  //BytesReturned
        );

	exit:
    return status;
}

