/*++
 Module Name:
    connection.c

 Environment:
    Kernel mode
--*/

#include "clisrv.h"


#if defined(EVENT_TRACING)
#include "connection.tmh"
#endif


#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, HfpRepeatReaderWaitForStop)
#pragma alloc_text (PAGE, HfpConnectionObjectWaitForAndUninitializeContinuousReader)
#pragma alloc_text (PAGE, HfpEvtConnectionObjectCleanup)
#pragma alloc_text (PAGE, HfpConnectionObjectRemoteDisconnectSynchronously)
#endif


void HfpRepeatReaderResubmitReadDpc(_In_ struct _KDPC  *Dpc, _In_opt_ PVOID  DeferredContext, _In_opt_ PVOID  SystemArgument1, _In_opt_ PVOID  SystemArgument2)
{
    HFP_REPEAT_READER* repeatReader;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument2);    

    repeatReader = (HFP_REPEAT_READER*) SystemArgument1;

    // HfpRepeatReaderSubmit will invoke contreader
    // failure callback in case of failure, so we don't check for failure here
    if (DeferredContext && repeatReader)
        (void) HfpRepeatReaderSubmit((HFPDEVICE_CONTEXT_HEADER*) DeferredContext, repeatReader); 
}


void HfpRepeatReaderPendingReadCompletion (_In_ WDFREQUEST  Request, _In_ WDFIOTARGET Target, _In_ PWDF_REQUEST_COMPLETION_PARAMS  Params, _In_ WDFCONTEXT  Context)
{
    HFP_REPEAT_READER* repeatReader;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Request);

    repeatReader = (HFP_REPEAT_READER*) Context;
       
    NT_ASSERT(repeatReader);

    status = Params->IoStatus.Status;

    if (NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONT_READER, "Pending read completion, RepeatReader: 0x%p, status: %d, Buffer: 0x%p, BufferSize: %d", repeatReader, Params->IoStatus.Status, repeatReader->TransferBrb.Buffer, repeatReader->TransferBrb.BufferSize);
           
        repeatReader->Connection->ContinuousReader.ReaderCompleteCallback(
            repeatReader->Connection->DevCtxHdr,
            repeatReader->Connection,
            repeatReader->TransferBrb.Buffer,
            repeatReader->TransferBrb.BufferSize
            );

    }
    else {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_CONT_READER, "Pending read completed with failure, RepeatReader: 0x%p, status: %d", repeatReader, Params->IoStatus.Status);
    }

    if (!NT_SUCCESS(status))
    {
        // Invoke reader failed callback only if status is something other than canceled. 
		// If the status is STATUS_CANCELLED this is because we have canceled repeat reader.
        if (STATUS_CANCELLED != status)
        {       
            // Invoke reader failed callback before setting the stop event
            // to ensure that connection object is alive during this callback
            repeatReader->Connection->ContinuousReader.ReaderFailedCallback (repeatReader->Connection->DevCtxHdr,  repeatReader->Connection);
        }
            
        // Set stop event because we are not resubmitting the reader
        KeSetEvent(&repeatReader->StopEvent, 0, FALSE);        
    }
    else
    {
        // Resubmit pending read. We use a Dpc to avoid recursing in this completion routine
        BOOLEAN ret = KeInsertQueueDpc(&repeatReader->ResubmitDpc, repeatReader, NULL);
        NT_ASSERT (ret); //we only expect one outstanding dpc
        UNREFERENCED_PARAMETER(ret); //ret remains unused in free build
    }
}


void HfpRepeatReaderUninitialize(_In_ HFP_REPEAT_READER* RepeatReader)
{
    if (NULL != RepeatReader->RequestPendingRead) {
        RepeatReader->RequestPendingRead = NULL;
        RepeatReader->Stopping = 0;        
    }    
}


_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS HfpRepeatReaderInitialize(_In_ HFP_CONNECTION* Connection, _In_ HFP_REPEAT_READER* RepeatReader, _In_ size_t BufferSize)
{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;

    // Create request object for pending read
    // Set connection object as the parent for the request
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = WdfObjectContextGetObject(Connection);

    status = WdfRequestCreate(&attributes, Connection->DevCtxHdr->IoTarget, &RepeatReader->RequestPendingRead);                    

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_CONT_READER, "Creating request for pending read failed, Status=%X", status);
        goto exit;
    }

    if (BufferSize <= 0) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_CONT_READER, "BufferSize has an invalid value: %I64d\n", BufferSize);
        status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    // Create memory object for the pending read
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = RepeatReader->RequestPendingRead;

    status = WdfMemoryCreate(
        &attributes,
        NonPagedPool,
        POOLTAG_HFPDRIVER,
        BufferSize,
        &RepeatReader->MemoryPendingRead,
        NULL //buffer
        );

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_CONT_READER, "Creating memory for pending read failed, Status=%X", status);
        goto exit;
    }

    // Initialize Dpc that we will use for resubmitting pending read
    KeInitializeDpc(&RepeatReader->ResubmitDpc, HfpRepeatReaderResubmitReadDpc, Connection->DevCtxHdr);

    // Initialize event used to wait for stop. This even is created as signaled. It gets cleared when request is submitted.
    KeInitializeEvent(&RepeatReader->StopEvent, NotificationEvent, TRUE);

    RepeatReader->Connection = Connection;
    
	exit:
    if (!NT_SUCCESS(status)) {
        HfpRepeatReaderUninitialize(RepeatReader);
    }
    
    return status;
}



_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS HfpRepeatReaderSubmit(_In_ HFPDEVICE_CONTEXT_HEADER* DevCtxHdr, _In_ HFP_REPEAT_READER* RepeatReader)
{
    NTSTATUS status, statusReuse;
    WDF_REQUEST_REUSE_PARAMS reuseParams;
    struct _BRB_SCO_TRANSFER *brb = &RepeatReader->TransferBrb;    

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONT_READER, "HfpRepeatReaderSubmit\n");

    DevCtxHdr->ProfileDrvInterface.BthReuseBrb((PBRB)brb, BRB_SCO_TRANSFER);

    WDF_REQUEST_REUSE_PARAMS_INIT(&reuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_UNSUCCESSFUL);
    statusReuse = WdfRequestReuse(RepeatReader->RequestPendingRead, &reuseParams);
    NT_ASSERT(NT_SUCCESS(statusReuse));
    UNREFERENCED_PARAMETER(statusReuse);

    // Check if we are stopping, if yes set StopEvent and exit.
    // After this point request is eligible for cancellation, so if this flag gets set after we check it, request will be canceled for stopping
    // the repeat reader and next time around we will stop when we are invoked again upon completion of canceled request.
    if (RepeatReader->Stopping)
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONT_READER, "Continuous reader 0x%p stopping\n", RepeatReader);        
        KeSetEvent(&RepeatReader->StopEvent, 0, FALSE);
        status = STATUS_SUCCESS;
        goto exit;
    }

    // Format request for SCO IN transfer
    status = HfpConnectionObjectFormatRequestForScoTransfer(
        RepeatReader->Connection,
        RepeatReader->RequestPendingRead,
        &brb,
        RepeatReader->MemoryPendingRead,
        SCO_TRANSFER_DIRECTION_IN
        );

    if (!NT_SUCCESS(status))
        goto exit;

    // Set a CompletionRoutine callback function.
    WdfRequestSetCompletionRoutine (RepeatReader->RequestPendingRead, HfpRepeatReaderPendingReadCompletion, RepeatReader);

    // Clear the stop event before sending the request. This is relevant only on start of the repeat reader 
    // (i.e. the first submission) this event eventually gets set only when repeat reader stops and not on every resubmission.
    KeClearEvent(&RepeatReader->StopEvent);

    if (FALSE == WdfRequestSend(RepeatReader->RequestPendingRead, DevCtxHdr->IoTarget, NULL)) {
        status = WdfRequestGetStatus(RepeatReader->RequestPendingRead);
        TraceEvents(TRACE_LEVEL_ERROR, DBG_CONT_READER, "Request send failed for request 0x%p, Brb 0x%p, Status code %d\n", RepeatReader->RequestPendingRead, brb, status);
        goto exit;
    }
    else {
        TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONT_READER, "Resubmited pending read with request 0x%p, Brb 0x%p\n", RepeatReader->RequestPendingRead, brb);        
    }

	exit:
    if (!NT_SUCCESS(status)) {
        // Invoke the reader failed callback before setting the event
        // to ensure that the connection object is alive during this callback
        RepeatReader->Connection->ContinuousReader.ReaderFailedCallback (RepeatReader->Connection->DevCtxHdr, RepeatReader->Connection);

        // If we failed to send pending read, set the event since we will not get completion callback
        KeSetEvent(&RepeatReader->StopEvent, 0, FALSE);
    }
    
    return status;
}



_IRQL_requires_max_(DISPATCH_LEVEL)
void HfpRepeatReaderCancel (_In_ HFP_REPEAT_READER* RepeatReader)
{
    // Signal that we are transitioning to stopped state so that we don't resubmit pending read
    InterlockedIncrement(&RepeatReader->Stopping);

    // Cancel the pending read.
    // If HfpRepeatReaderSubmit misses the setting of Stopping flag, cancel would cause the request to complete 
	// quickly and repeat reader will subsequently be stopped.
    WdfRequestCancelSentRequest (RepeatReader->RequestPendingRead);    
}



_IRQL_requires_max_(PASSIVE_LEVEL)
void HfpRepeatReaderWaitForStop (_In_ HFP_REPEAT_READER* RepeatReader)
{
    PAGED_CODE();

    // Wait for pending read to complete
    KeWaitForSingleObject (&RepeatReader->StopEvent, Executive, KernelMode, FALSE, NULL);    
}



//====================================================================
// Connection object functions
//====================================================================


_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS HfpConnectionObjectInitializeContinuousReader(
    _In_ HFP_CONNECTION* Connection,
    _In_ PFN_HFP_CONNECTION_OBJECT_CONTREADER_READ_COMPLETE HfpConnectionObjectContReaderReadCompleteCallback,
    _In_ PFN_HFP_CONNECTION_OBJECT_CONTREADER_FAILED        HfpConnectionObjectContReaderFailedCallback,
    _In_ size_t BufferSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    UINT i;

    Connection->ContinuousReader.ReaderCompleteCallback = HfpConnectionObjectContReaderReadCompleteCallback;
    Connection->ContinuousReader.ReaderFailedCallback	= HfpConnectionObjectContReaderFailedCallback;
        
    for (i = 0; i < HFP_NUM_CONTINUOUS_READERS; i++)
    {
        status = HfpRepeatReaderInitialize (Connection, &Connection->ContinuousReader.RepeatReaders[i], BufferSize);
        if (!NT_SUCCESS(status))
            break;
        ++Connection->ContinuousReader.InitializedReadersCount;
    }

    // In case of error (as well as on success) HfpEvtConnectionObjectCleanup will uninitialized readers
    return status;
}



_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS HfpConnectionObjectContinuousReaderSubmitReaders(_In_ HFP_CONNECTION* Connection)
{
    NTSTATUS status = STATUS_SUCCESS;
    UINT i;

    NT_ASSERT (Connection->ContinuousReader.InitializedReadersCount <= HFP_NUM_CONTINUOUS_READERS);
    
    for (i = 0; i < Connection->ContinuousReader.InitializedReadersCount; i++)  {
        status = HfpRepeatReaderSubmit (Connection->DevCtxHdr, &Connection->ContinuousReader.RepeatReaders[i]);
        if (!NT_SUCCESS(status))
            goto exit;
    }

	exit:
    if (!NT_SUCCESS(status)) {
        // Cancel any submitted continuous readers
        // We make sure that it is safe to call cancel on unsubmitted readers
        HfpConnectionObjectContinuousReaderCancelReaders(Connection);
    }
    
    return status;
}



_IRQL_requires_max_(DISPATCH_LEVEL)
void HfpConnectionObjectContinuousReaderCancelReaders (_In_ HFP_CONNECTION* Connection)
{
    UINT i;

    NT_ASSERT (Connection->ContinuousReader.InitializedReadersCount <= HFP_NUM_CONTINUOUS_READERS);

    for (i = 0; i < Connection->ContinuousReader.InitializedReadersCount; i++)
        HfpRepeatReaderCancel(&Connection->ContinuousReader.RepeatReaders[i]);
}



_IRQL_requires_max_(PASSIVE_LEVEL)
void HfpConnectionObjectWaitForAndUninitializeContinuousReader (_In_ HFP_CONNECTION* Connection)
{
    UINT i;

    PAGED_CODE();

    NT_ASSERT (Connection->ContinuousReader.InitializedReadersCount <= HFP_NUM_CONTINUOUS_READERS);
    
    for (i = 0; i < Connection->ContinuousReader.InitializedReadersCount; i++) {
        HFP_REPEAT_READER* repeatReader = &Connection->ContinuousReader.RepeatReaders[i];

        HfpRepeatReaderWaitForStop(repeatReader); // It is OK to wait on unsubmitted readers
        HfpRepeatReaderUninitialize(repeatReader);
    }    
}



void HfpEvtConnectionObjectCleanup (_In_ WDFOBJECT  ConnectionObject)
{
    HFP_CONNECTION* connection = GetConnectionObjectContext(ConnectionObject);

    PAGED_CODE();
        
    HfpConnectionObjectWaitForAndUninitializeContinuousReader(connection);
    KeWaitForSingleObject(&connection->DisconnectEvent, Executive, KernelMode, FALSE, NULL);
    WdfObjectDelete(connection->ConnectDisconnectRequest);
}



_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS HfpConnectionObjectInit (_In_ WDFOBJECT ConnectionObject, _In_ HFPDEVICE_CONTEXT_HEADER* DevCtxHdr)
{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;
    HFP_CONNECTION* connection = GetConnectionObjectContext(ConnectionObject);

    connection->DevCtxHdr = DevCtxHdr;
    connection->ConnectionState = ConnectionStateInitialized;

    // Initialize spinlock
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = ConnectionObject;

    status = WdfSpinLockCreate (&attributes, &connection->ConnectionLock);
    if (!NT_SUCCESS(status))
        goto exit;                

    // Create connect/disconnect request
    status = WdfRequestCreate (&attributes, DevCtxHdr->IoTarget, &connection->ConnectDisconnectRequest);
    if (!NT_SUCCESS(status))
        return status;

    // Initialize event
    KeInitializeEvent(&connection->DisconnectEvent, NotificationEvent, TRUE);

    // Initialize list entry
    InitializeListHead(&connection->ConnectionListEntry);
    connection->ConnectionState = ConnectionStateInitialized;

	exit:
    return status;
}



_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS HfpConnectionObjectCreate (_In_ HFPDEVICE_CONTEXT_HEADER* DevCtxHdr, _In_ WDFOBJECT ParentObject, _Out_ WDFOBJECT*  ConnectionObject)
{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDFOBJECT connectionObject = NULL;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, HFP_CONNECTION);
    attributes.ParentObject		  = ParentObject;    
    attributes.EvtCleanupCallback = HfpEvtConnectionObjectCleanup;

    // We set execution level to passive so that we get cleanup at passive level 
	// where we can wait for continuous readers to run down and for completion of disconnect
    attributes.ExecutionLevel = WdfExecutionLevelPassive; 

    status = WdfObjectCreate (&attributes, &connectionObject);
    if (!NT_SUCCESS(status))  {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_CONNECT, "WdfObjectCreate for connection object failed, Status=%X", status);
        goto exit;
    }

    status = HfpConnectionObjectInit (connectionObject, DevCtxHdr);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_CONNECT, "Context initialize for connection object failed, ConnectionObject 0x%p, Status code %d\n", connectionObject, status);
        goto exit;
    }

    *ConnectionObject = connectionObject;
    
	exit:
    if(!NT_SUCCESS(status) && connectionObject) {
        WdfObjectDelete(connectionObject);
    }
    
    return status;
}



void HfpConnectionObjectDisconnectCompletion(
    _In_ WDFREQUEST  Request,
    _In_ WDFIOTARGET  Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS  Params,
    _In_ WDFCONTEXT  Context
    )
{
    HFP_CONNECTION* connection = (HFP_CONNECTION*) Context;

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Params);

    WdfSpinLockAcquire(connection->ConnectionLock);
    connection->ConnectionState = ConnectionStateDisconnected;
    WdfSpinLockRelease(connection->ConnectionLock);

    // Disconnect complete, set the event
    KeSetEvent(&connection->DisconnectEvent, 0, FALSE);    
}



_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN HfpConnectionObjectRemoteDisconnect(_In_ HFPDEVICE_CONTEXT_HEADER* DevCtxHdr, _In_ HFP_CONNECTION* Connection)
{
	struct _BRB_SCO_CLOSE_CHANNEL * disconnectBrb;

    // Cancel continuous readers for the connection
    HfpConnectionObjectContinuousReaderCancelReaders(Connection);
    
    WdfSpinLockAcquire(Connection->ConnectionLock);
    
    if (Connection->ConnectionState == ConnectionStateConnecting) 
    {
        // If the connection is not completed yet set the state to disconnecting.
        // In such case we should send CLOSE_CHANNEL Brb down after we receive connect completion.
        Connection->ConnectionState = ConnectionStateDisconnecting;

        // Clear event to indicate that we are in disconnecting state. It will be set when disconnect is completed
        KeClearEvent(&Connection->DisconnectEvent);

        WdfSpinLockRelease(Connection->ConnectionLock);
        return TRUE;

    } 
    else if (Connection->ConnectionState != ConnectionStateConnected)
    {
        // Do nothing if we are not connected
        WdfSpinLockRelease(Connection->ConnectionLock);
        return FALSE;
    }

    Connection->ConnectionState = ConnectionStateDisconnecting;
    WdfSpinLockRelease(Connection->ConnectionLock);

    // We are now sending the disconnect, so clear the event.
    KeClearEvent(&Connection->DisconnectEvent);

    DevCtxHdr->ProfileDrvInterface.BthReuseBrb(&Connection->ConnectDisconnectBrb, BRB_SCO_CLOSE_CHANNEL);

    disconnectBrb = (struct _BRB_SCO_CLOSE_CHANNEL *) &(Connection->ConnectDisconnectBrb);
    
    disconnectBrb->BtAddress = Connection->RemoteAddress;
    disconnectBrb->ChannelHandle = Connection->ChannelHandle;

    // The BRB can fail with STATUS_DEVICE_DISCONNECT if the device is already disconnected, hence we don't assert for success
    (void) HfpSharedSendBrbAsync(
        DevCtxHdr->IoTarget, 
        Connection->ConnectDisconnectRequest, 
        (PBRB) disconnectBrb,
        sizeof(*disconnectBrb),
        HfpConnectionObjectDisconnectCompletion,
        Connection
        );    

    return TRUE;
}



_IRQL_requires_max_(PASSIVE_LEVEL)
void HfpConnectionObjectRemoteDisconnectSynchronously (_In_ HFPDEVICE_CONTEXT_HEADER* DevCtxHdr, _In_ HFP_CONNECTION* Connection)
{
    PAGED_CODE();
    HfpConnectionObjectRemoteDisconnect(DevCtxHdr, Connection);
    KeWaitForSingleObject (&Connection->DisconnectEvent, Executive, KernelMode, FALSE, NULL);    
}



_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS FormatRequestWithBrb (_In_ WDFIOTARGET IoTarget, _In_ WDFREQUEST Request, _In_ PBRB Brb, _In_ size_t BrbSize)
{
    NTSTATUS status         = BTH_ERROR_SUCCESS;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDFMEMORY memoryArg1    = NULL;

    if (BrbSize <= 0) {        
        TraceEvents(TRACE_LEVEL_ERROR, DBG_UTIL, "BrbSize has an invalid value: %I64d\n", BrbSize);
        status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = Request;

    status = WdfMemoryCreatePreallocated (&attributes, Brb, BrbSize, &memoryArg1);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_UTIL, "Creating preallocated memory for Brb 0x%p failed, Request to be formatted 0x%p, Status code %d\n", Brb, Request, status);
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
        TraceEvents(TRACE_LEVEL_ERROR, DBG_UTIL, "Formatting request 0x%p with Brb 0x%p failed, Status code %d\n", Request, Brb, status);
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

    if (NULL == *Brb) {
        brb = (struct _BRB_SCO_TRANSFER*) Connection->DevCtxHdr->ProfileDrvInterface.BthAllocateBrb (BRB_SCO_TRANSFER, POOLTAG_HFPDRIVER);

        if (!brb) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE, "Failed to allocate BRB_SCO_TRANSFER, returning status code %d\n", status);
            goto exit;
        }
        brbAllocatedLocally = TRUE;
    }
    else {
        brb = *Brb;
        Connection->DevCtxHdr->ProfileDrvInterface.BthReuseBrb((PBRB)brb, BRB_SCO_TRANSFER);
    }

    brb->BtAddress = Connection->RemoteAddress;
    brb->BufferMDL = NULL;
    brb->Buffer    = WdfMemoryGetBuffer(Memory, &bufferSize);

    __analysis_assume(bufferSize <= (ULONG)(-1));
    if (bufferSize > (ULONG)(-1))
    {
        status = STATUS_BUFFER_OVERFLOW;
        TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE, "Buffer passed in longer than max ULONG, returning status code %d\n", status);
        goto exit;        
    }
    
    brb->BufferSize    = (ULONG) bufferSize;
    brb->ChannelHandle = Connection->ChannelHandle;
    brb->TransferFlags = TransferFlags;

    //TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE,  "Format Request For SCO: bufferSize = %d\n", bufferSize);
    status = FormatRequestWithBrb (Connection->DevCtxHdr->IoTarget, Request, (PBRB) brb, sizeof(*brb));
    
    if (!NT_SUCCESS(status))
        goto exit;

    if (NULL == *Brb)
        *Brb = brb;

	exit:
    if (!NT_SUCCESS(status) && brb && brbAllocatedLocally)
        Connection->DevCtxHdr->ProfileDrvInterface.BthFreeBrb((PBRB)brb);
    
    return status;
}
