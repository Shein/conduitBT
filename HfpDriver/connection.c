/*++
 Module Name:
    connection.c

 Abstract:
	Connection object functions

 Environment:
    Kernel mode
--*/

#include "driver.h"
#include "device.h"
#include "connection.h"
#include "clisrv.h"


#if defined(EVENT_TRACING)
#include "connection.tmh"
#endif


#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, HfpEvtConnectionObjectCleanup)
#pragma alloc_text (PAGE, HfpConnectionObjectRemoteDisconnectSynchronously)
#endif



void HfpEvtConnectionObjectCleanup (_In_ WDFOBJECT  ConnectionObject)
{
    HFP_CONNECTION* connection = GetConnectionObjectContext(ConnectionObject);

    PAGED_CODE();
        
    // Here may be "WaitForAndUninitializeContinuousReader for the connection"
    // We do not have continuous readers in the driver
    KeWaitForSingleObject(&connection->DisconnectEvent, Executive, KernelMode, FALSE, NULL);
    WdfObjectDelete(connection->ConnectDisconnectRequest);
}



_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS HfpConnectionObjectInit (_In_ WDFOBJECT ConnectionObject, _In_ HFPDEVICE_CONTEXT* devCtx, _In_ WDFOBJECT parentObject)
{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;
    HFP_CONNECTION* connection = GetConnectionObjectContext(ConnectionObject);

    // Initialize spinlock
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = ConnectionObject;

    status = WdfSpinLockCreate (&attributes, &connection->ConnectionLock);
    if (!NT_SUCCESS(status))
        goto exit;                

    // Create connect/disconnect request
    status = WdfRequestCreate (&attributes, devCtx->IoTarget, &connection->ConnectDisconnectRequest);
    if (!NT_SUCCESS(status))
        return status;

    // Initialize event
    KeInitializeEvent(&connection->DisconnectEvent, NotificationEvent, TRUE);

    // Initialize list entry
    connection->DevCtx = devCtx;
	connection->FileObject = (WDFFILEOBJECT) parentObject;		// Our connection parent is always FileObject
    connection->ConnectionState = ConnectionStateInitialized;

	exit:
    return status;
}



_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS HfpConnectionObjectCreate (_In_ HFPDEVICE_CONTEXT* devCtx, _In_ WDFOBJECT parentObject, _Out_ WDFOBJECT* ConnectionObject)
{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDFOBJECT connectionObject = 0;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, HFP_CONNECTION);
    attributes.ParentObject		  = parentObject;    
    attributes.EvtCleanupCallback = HfpEvtConnectionObjectCleanup;

    // We set execution level to passive so that we get cleanup at passive level 
	// where we can wait for continuous readers to run down and for completion of disconnect
    attributes.ExecutionLevel = WdfExecutionLevelPassive; 

    status = WdfObjectCreate (&attributes, &connectionObject);
    if (!NT_SUCCESS(status))  {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_CONNECT, "WdfObjectCreate for connection object failed, Status=%X", status);
        goto exit;
    }

    status = HfpConnectionObjectInit (connectionObject, devCtx, parentObject);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_CONNECT, "HfpConnectionObjectInit failed, Status %X", status);
        goto exit;
    }

    *ConnectionObject = connectionObject;
    
	exit:
    if(!NT_SUCCESS(status) && connectionObject)
        WdfObjectDelete(connectionObject);
    
    return status;
}



void HfpConnectionObjectDisconnectCompletion(
    _In_ WDFREQUEST   Request,
    _In_ WDFIOTARGET  Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS  Params,
    _In_ WDFCONTEXT  Context
    )
{
    HFP_CONNECTION* connection = (HFP_CONNECTION*) Context;
	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_PNP, "HfpConnectionObjectDisconnectCompletion");

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Params);

    WdfSpinLockAcquire(connection->ConnectionLock);
    connection->ConnectionState = ConnectionStateDisconnected;
	NT_ASSERT (connection->FileObject);
	GetFileContext(connection->FileObject)->Connection = 0;
    WdfSpinLockRelease(connection->ConnectionLock);

    // Disconnect complete, set the event
    KeSetEvent(&connection->DisconnectEvent, 0, FALSE);    
}



_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN HfpConnectionObjectRemoteDisconnect(_In_ HFPDEVICE_CONTEXT* devCtx, _In_ HFP_CONNECTION* connection)
{
	struct _BRB_SCO_CLOSE_CHANNEL * disconnectBrb;

	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_PNP, "HfpConnectionObjectRemoteDisconnect, connection=%X", connection);

    // Here may be "Continuous readers cancellation for the connection"
    // We do not have continuous readers in the driver
    
    WdfSpinLockAcquire(connection->ConnectionLock);
    
    if (connection->ConnectionState == ConnectionStateConnecting) 
    {
        // If the connection is not completed yet set the state to disconnecting.
        // In such case we should send CLOSE_CHANNEL Brb down after we receive connect completion.
        connection->ConnectionState = ConnectionStateDisconnecting;

        // Clear event to indicate that we are in disconnecting state. It will be set when disconnect is completed
        KeClearEvent(&connection->DisconnectEvent);
        WdfSpinLockRelease(connection->ConnectionLock);
        return TRUE;

    } 
    else if (connection->ConnectionState != ConnectionStateConnected)
    {
        // Do nothing if we are not connected
        WdfSpinLockRelease(connection->ConnectionLock);
        return FALSE;
    }

    connection->ConnectionState = ConnectionStateDisconnecting;
    WdfSpinLockRelease(connection->ConnectionLock);

    // We are now sending the disconnect, so clear the event.
    KeClearEvent(&connection->DisconnectEvent);

    devCtx->ProfileDrvInterface.BthReuseBrb(&connection->ConnectDisconnectBrb, BRB_SCO_CLOSE_CHANNEL);

    disconnectBrb = (struct _BRB_SCO_CLOSE_CHANNEL*) &(connection->ConnectDisconnectBrb);

    disconnectBrb->BtAddress     = connection->RemoteAddress;
    disconnectBrb->ChannelHandle = connection->ChannelHandle;

    // The BRB can fail with STATUS_DEVICE_DISCONNECT if the device is already disconnected, hence we don't assert for success
    HfpSharedSendBrbAsync(
        devCtx->IoTarget, 
        connection->ConnectDisconnectRequest, 
        (PBRB) disconnectBrb,
        sizeof(*disconnectBrb),
        HfpConnectionObjectDisconnectCompletion,
        connection
        );    

    return TRUE;
}



_IRQL_requires_max_(PASSIVE_LEVEL)
void HfpConnectionObjectRemoteDisconnectSynchronously (_In_ HFPDEVICE_CONTEXT* devCtx, _In_ HFP_CONNECTION* Connection)
{
    PAGED_CODE();
    HfpConnectionObjectRemoteDisconnect(devCtx, Connection);
    KeWaitForSingleObject (&Connection->DisconnectEvent, Executive, KernelMode, FALSE, NULL);    
}



_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS FormatRequestWithBrb (_In_ WDFIOTARGET IoTarget, _In_ WDFREQUEST Request, _In_ PBRB Brb, _In_ size_t BrbSize)
{
    WDF_OBJECT_ATTRIBUTES attributes;
    NTSTATUS  status     = BTH_ERROR_SUCCESS;
    WDFMEMORY memoryArg1 = NULL;

    if (BrbSize <= 0) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_UTIL, "BrbSize has an invalid value: %I64d", BrbSize);
        status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = Request;

    status = WdfMemoryCreatePreallocated (&attributes, Brb, BrbSize, &memoryArg1);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_UTIL, "Creating preallocated memory for Brb 0x%p failed, Request to be formatted 0x%p, Status %X", Brb, Request, status);
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
        TraceEvents(TRACE_LEVEL_ERROR, DBG_UTIL, "Formatting request 0x%p with Brb 0x%p failed, Status %X", Request, Brb, status);
        goto exit;
    }

	exit:
    return status;
}


_IRQL_requires_max_(DISPATCH_LEVEL)
_When_(return==0, _At_(*Brb, __drv_allocatesMem(Mem)))
NTSTATUS HfpConnectionObjectFormatRequestForScoTransfer(
    _In_ HFP_CONNECTION* Connection,
    _In_ WDFREQUEST Request,
    _Inout_ struct _BRB_SCO_TRANSFER ** Brb,
    _In_ WDFMEMORY Memory,
    _In_ ULONG TransferFlags //flags include direction of transfer
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    struct _BRB_SCO_TRANSFER *brb = NULL;
    size_t bufferSize;
    BOOLEAN brbAllocatedLocally = FALSE; //whether this function allocated the BRB

    WdfSpinLockAcquire(Connection->ConnectionLock);

    if (Connection->ConnectionState != ConnectionStateConnected) {
        status = STATUS_CONNECTION_DISCONNECTED;
        WdfSpinLockRelease(Connection->ConnectionLock);
        goto exit;
    }

    WdfSpinLockRelease(Connection->ConnectionLock);

    if (!(*Brb)) {
        brb = (struct _BRB_SCO_TRANSFER*) Connection->DevCtx->ProfileDrvInterface.BthAllocateBrb (BRB_SCO_TRANSFER, POOLTAG_HFPDRIVER);

        if (!brb) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE, "Failed to allocate BRB_SCO_TRANSFER, Status %X", status);
            goto exit;
        }
        brbAllocatedLocally = TRUE;
    }
    else {
        brb = *Brb;
        Connection->DevCtx->ProfileDrvInterface.BthReuseBrb((PBRB)brb, BRB_SCO_TRANSFER);
    }

    brb->BtAddress = Connection->RemoteAddress;
    brb->BufferMDL = NULL;
    brb->Buffer    = WdfMemoryGetBuffer(Memory, &bufferSize);

    __analysis_assume(bufferSize <= (ULONG)(-1));
    if (bufferSize > (ULONG)(-1))
    {
        status = STATUS_BUFFER_OVERFLOW;
        TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE, "Buffer passed in longer than max ULONG, Status %X", status);
        goto exit;        
    }
    
    brb->BufferSize    = (ULONG) bufferSize;
    brb->ChannelHandle = Connection->ChannelHandle;
    brb->TransferFlags = TransferFlags;

    //TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE,  "Format Request For SCO: bufferSize = %d\n", bufferSize);
    status = FormatRequestWithBrb (Connection->DevCtx->IoTarget, Request, (PBRB) brb, sizeof(*brb));
    
    if (!NT_SUCCESS(status))
        goto exit;

    if (!(*Brb))
        *Brb = brb;

	exit:
    if (!NT_SUCCESS(status) && brb && brbAllocatedLocally)
        Connection->DevCtx->ProfileDrvInterface.BthFreeBrb((PBRB)brb);
    
    return status;
}
