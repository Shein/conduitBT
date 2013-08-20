/*++

Module Name:
    Server.c

Abstract:
    Contains Bluetooth server functionality.
    
Environment:
    Kernel mode only
--*/


#include "device.h"
#include "clisrv.h"
#include "server.h"
#include "sdp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, HfpSrvPublishSdpRecord)
#pragma alloc_text (PAGE, HfpSrvRemoveSdpRecord)
#endif


static void InsertConnectionEntryLocked (HFPDEVICE_CONTEXT* devCtx, PLIST_ENTRY ple)
{
    WdfSpinLockAcquire(devCtx->ConnectionListLock);
    InsertTailList(&devCtx->ConnectionList, ple);
    WdfSpinLockRelease(devCtx->ConnectionListLock);    
}

static void RemoveConnectionEntryLocked (HFPDEVICE_CONTEXT* devCtx, PLIST_ENTRY ple)
{
    WdfSpinLockAcquire(devCtx->ConnectionListLock);
    RemoveEntryList(ple);
    WdfSpinLockRelease(devCtx->ConnectionListLock);    
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS HfpSrvPublishSdpRecord (_In_ WDFDEVICE Device)
{
    NTSTATUS status, statusReuse;
    WDF_MEMORY_DESCRIPTOR inMemDesc;
    WDF_MEMORY_DESCRIPTOR outMemDesc;
    WDF_REQUEST_REUSE_PARAMS ReuseParams;
    HANDLE_SDP sdpRecordHandle;
    HFPDEVICE_CONTEXT* devCtx = GetClientDeviceContext(Device);

	// -----------------------------------------------------------------------------------------------
	// Preparing sdpRecordStream, sdpRecordLength 

    BTHDDI_SDP_NODE_INTERFACE  sdpNodeInterface;
    BTHDDI_SDP_PARSE_INTERFACE sdpParseInterface;
    UCHAR* sdpRecordStream = NULL;	// SDP record to publish
    ULONG  sdpRecordLength = 0;		// SDP record length

    status = WdfFdoQueryForInterface(
        Device,
        &GUID_BTHDDI_SDP_PARSE_INTERFACE, 
        (PINTERFACE) (&sdpParseInterface),
        sizeof(sdpParseInterface), 
        BTHDDI_SDP_PARSE_INTERFACE_VERSION_FOR_QI, 
        NULL
        );
                
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "QueryInterface failed for Interface SDP parse, Status %X\n", status);
        goto exit;
    }

    status = WdfFdoQueryForInterface(
        Device,
        &GUID_BTHDDI_SDP_NODE_INTERFACE, 
        (PINTERFACE) (&sdpNodeInterface),
        sizeof(sdpNodeInterface), 
        BTHDDI_SDP_NODE_INTERFACE_VERSION_FOR_QI, 
        NULL
        );
                
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "QueryInterface failed for Interface SDP node, Status %X\n", status);
        goto exit;
    }

    status = CreateSdpRecord(
        &sdpNodeInterface,
        &sdpParseInterface,
        HFP_CLASS_ID,
        BthHfpDriverName,
        &sdpRecordStream,
        &sdpRecordLength
        );

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "CreateSdpRecord failed, Status %X\n", status);
        goto exit;
    }
    
	// -----------------------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------------------

    PAGED_CODE();

    WDF_REQUEST_REUSE_PARAMS_INIT(&ReuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_NOT_SUPPORTED);
    statusReuse = WdfRequestReuse(devCtx->Header.Request, &ReuseParams);    
    NT_ASSERT(NT_SUCCESS(statusReuse));
    UNREFERENCED_PARAMETER(statusReuse);

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER (&inMemDesc, sdpRecordStream, sdpRecordLength);
    RtlZeroMemory( &sdpRecordHandle, sizeof(HANDLE_SDP) );
    
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER (&outMemDesc, &sdpRecordHandle, sizeof(HANDLE_SDP));

    status = WdfIoTargetSendIoctlSynchronously(
        devCtx->Header.IoTarget,
        devCtx->Header.Request,
        IOCTL_BTH_SDP_SUBMIT_RECORD,
        &inMemDesc,
        &outMemDesc,
        NULL,   //sendOptions
        NULL    //bytesReturned
        );

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,  "IOCTL_BTH_SDP_SUBMIT_RECORD failed, Status=%X", status);
        goto exit;
    }

    devCtx->SdpRecordHandle = sdpRecordHandle;
    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, "IOCTL_BTH_SDP_SUBMIT_RECORD completed, handle = %X", sdpRecordHandle);

	exit:
    if (sdpRecordStream)
        FreeSdpRecord(sdpRecordStream);
    
    return status;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
void HfpSrvRemoveSdpRecord (_In_ HFPDEVICE_CONTEXT* devCtx)
{
    NTSTATUS status, statusReuse;
    WDF_MEMORY_DESCRIPTOR inMemDesc;
    WDF_REQUEST_REUSE_PARAMS ReuseParams;
    HANDLE_SDP sdpRecordHandle;

    PAGED_CODE();
    
    WDF_REQUEST_REUSE_PARAMS_INIT(&ReuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_NOT_SUPPORTED);
    statusReuse =  WdfRequestReuse(devCtx->Header.Request, &ReuseParams);    
    NT_ASSERT(NT_SUCCESS(statusReuse));
    UNREFERENCED_PARAMETER(statusReuse);

    sdpRecordHandle = devCtx->SdpRecordHandle;

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER (&inMemDesc, &sdpRecordHandle, sizeof(HANDLE_SDP));
    
    status = WdfIoTargetSendIoctlSynchronously(
        devCtx->Header.IoTarget,
        devCtx->Header.Request,
        IOCTL_BTH_SDP_REMOVE_RECORD,
        &inMemDesc,
        0,		//outMemDesc
        0,		//sendOptions
        0		//bytesReturned
        );

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "IOCTL_BTH_SDP_REMOVE_RECORD failed, Status=%X", status);

        // Send does not fail for resource reasons
        NT_ASSERT(FALSE);
        goto exit;
    }

    devCtx->SdpRecordHandle = HANDLE_SDP_NULL;

	exit:
    return;    
}



void HfpSrvRemoteConnectResponseCompletion (_In_ WDFREQUEST  Request, _In_ WDFIOTARGET  Target, _In_ PWDF_REQUEST_COMPLETION_PARAMS  Params, _In_ WDFCONTEXT Context)
{
    NTSTATUS						status = Params->IoStatus.Status;
    struct _BRB_SCO_OPEN_CHANNEL*	brb = (struct _BRB_SCO_OPEN_CHANNEL*) Context;
    WDFOBJECT						connectionObject = (WDFOBJECT) brb->Hdr.ClientContext[0];	// We receive connection object as the context in the BRB
    HFP_CONNECTION*					connection		 = GetConnectionObjectContext(connectionObject);

    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Request); //we reuse the request, hence it is not needed here
   
    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT,  "Connection completion, status: %d", status);        

    if (NT_SUCCESS(status))
    {
        connection->ChannelHandle = brb->ChannelHandle;
        connection->RemoteAddress = brb->BtAddress;

        TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, "Connection established with client");

        // Check if we already received a disconnect request and if so disconnect
        WdfSpinLockAcquire(connection->ConnectionLock);

        if (connection->ConnectionState == ConnectionStateDisconnecting) 
        {
            // We allow transition to disconnected state only from connected state.
            // If we are in disconnection state this means that we were waiting for connect to complete 
			// before we can send disconnect down.
            // Set the state to Connected and call HfpConnectionObjectRemoteDisconnect
            connection->ConnectionState = ConnectionStateConnected;
            WdfSpinLockRelease(connection->ConnectionLock);

            TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, "Remote connect response completion: disconnect received for connection 0x%p", connection);
    
            HfpConnectionObjectRemoteDisconnect(connection->DevCtxHdr, connection);
        }
        else {
            connection->ConnectionState = ConnectionStateConnected;
            WdfSpinLockRelease(connection->ConnectionLock);

            TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT,  "Connection completed, connection: 0x%p", connection);

            // Call HfpSrvConnectionStateConnected to perform post connect processing
            // (namely initializing continuous readers)
            status = HfpSrvConnectionStateConnected(connectionObject);
            if (!NT_SUCCESS(status)) {
                // If the post connect processing failed, let us disconnect
                HfpConnectionObjectRemoteDisconnect(connection->DevCtxHdr, connection);
            }            
        }
    }
    else
    {
        BOOLEAN setDisconnectEvent = FALSE;
        
        WdfSpinLockAcquire(connection->ConnectionLock);
        if (connection->ConnectionState == ConnectionStateDisconnecting) 
            setDisconnectEvent = TRUE;

        connection->ConnectionState = ConnectionStateConnectFailed;
        WdfSpinLockRelease(connection->ConnectionLock);

        if (setDisconnectEvent)
            KeSetEvent (&connection->DisconnectEvent,0,FALSE);
    }

    if (!NT_SUCCESS(status)) {        
        RemoveConnectionEntryLocked((HFPDEVICE_CONTEXT*) connection->DevCtxHdr, &connection->ConnectionListEntry);
        WdfObjectDelete(connectionObject);        
    }
    
    return;    
}



_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS HfpSrvSendConnectResponse(_In_ HFPDEVICE_CONTEXT* devCtx, _In_ SCO_INDICATION_PARAMETERS* ConnectParams)
{
    NTSTATUS					status, statusReuse;
    WDF_REQUEST_REUSE_PARAMS	reuseParams;
    WDFOBJECT					connectionObject;
	HFP_CONNECTION*				connection = 0;
	struct _BRB_SCO_OPEN_CHANNEL* brb;
    
    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, "HfpSrvSendConnectResponse: LinkType = %X", ConnectParams->Parameters.Connect.Request.LinkType);

    // We create the connection object as the first step so that if we receive 
    // remove before connect response is completed we can wait for connection and disconnect.
    status = HfpConnectionObjectCreate(&devCtx->Header, devCtx->Header.Device, &connectionObject);

    if (!NT_SUCCESS(status))
        goto exit;

    connection = GetConnectionObjectContext(connectionObject);
    connection->ConnectionState = ConnectionStateConnecting;

    // Insert the connection object in the connection list that we track
    InsertConnectionEntryLocked(devCtx, &connection->ConnectionListEntry);

    WDF_REQUEST_REUSE_PARAMS_INIT(&reuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_NOT_SUPPORTED);
    statusReuse = WdfRequestReuse(connection->ConnectDisconnectRequest, &reuseParams);
    NT_ASSERT(NT_SUCCESS(statusReuse));
    UNREFERENCED_PARAMETER(statusReuse);
    
    brb = (struct _BRB_SCO_OPEN_CHANNEL*) &(connection->ConnectDisconnectBrb);
    devCtx->Header.ProfileDrvInterface.BthReuseBrb((PBRB)brb, BRB_SCO_OPEN_CHANNEL_RESPONSE);

    brb->Hdr.ClientContext[0] = connectionObject;
    brb->BtAddress			  = ConnectParams->BtAddress;
    brb->ChannelHandle		  = ConnectParams->ConnectionHandle;
    brb->Response			  = CONNECT_RSP_RESULT_SUCCESS;
    brb->ChannelFlags		  = SCO_CF_LINK_SUPPRESS_PIN;

	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, "Incoming SCO ReceiveBandwidth = %d", brb->ReceiveBandwidth);
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, "Incoming SCO PacketType = %X",		brb->PacketType);
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, "Incoming SCO ContentFormat = %X",	brb->ContentFormat);
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, "Incoming SCO MaxLatency = %X",		brb->MaxLatency);

    // Get notifications about disconnect
    brb->CallbackFlags	 = CALLBACK_DISCONNECT;
    brb->Callback		 = &HfpSrvConnectionIndicationCallback;
    brb->CallbackContext = connectionObject;
    brb->ReferenceObject = (void*) WdfDeviceWdmGetDeviceObject(devCtx->Header.Device);

    status = HfpSharedSendBrbAsync(devCtx->Header.IoTarget, connection->ConnectDisconnectRequest, (PBRB)brb, sizeof(*brb), HfpSrvRemoteConnectResponseCompletion, brb);

    if (!NT_SUCCESS(status))
        TraceEvents(TRACE_LEVEL_ERROR, DBG_CONNECT, "HfpSharedSendBrbAsync failed, status = %d", status);                
    
	exit:
    if (!NT_SUCCESS(status) && connectionObject) {
        // If we failed to connect remove connection from list and delete the object.
        // Connection should not be NULL, if connectionObject is not NULL since first thing we do 
		// after creating connectionObject is to get context which gives us connection
        NT_ASSERT (connection);

        if (connection != NULL) {
            connection->ConnectionState = ConnectionStateConnectFailed;
            RemoveConnectionEntryLocked ((HFPDEVICE_CONTEXT*) connection->DevCtxHdr, &connection->ConnectionListEntry);
        }

        WdfObjectDelete(connectionObject);
    }
    return status;
}



_IRQL_requires_max_(DISPATCH_LEVEL)
void HfpSrvDisconnectConnection (_In_ HFP_CONNECTION * connection)
{
    if (HfpConnectionObjectRemoteDisconnect(connection->DevCtxHdr, connection)) {
        RemoveConnectionEntryLocked ((HFPDEVICE_CONTEXT*) connection->DevCtxHdr, &connection->ConnectionListEntry);
        WdfObjectDelete (WdfObjectContextGetObject(connection));    
    }
    // else connection is already disconnected
}



_IRQL_requires_max_(DISPATCH_LEVEL)
void HfpSrvConnectionIndicationCallback (_In_ void* Context, _In_ SCO_INDICATION_CODE Indication, _In_ SCO_INDICATION_PARAMETERS* Parameters)
{
	HFP_CONNECTION *connection;
    UNREFERENCED_PARAMETER(Parameters);

    switch (Indication)
    {
        case ScoIndicationAddReference:
        case ScoIndicationReleaseReference:
            break;

        case ScoIndicationRemoteConnect:
            // We don't expect connect on this callback
            NT_ASSERT(FALSE);
            break;

        case ScoIndicationRemoteDisconnect:
            connection = GetConnectionObjectContext((WDFOBJECT)Context /*connectionObject*/);
            HfpSrvDisconnectConnection(connection);
            break;

        default:
            // We don't expect any other indications on this callback
            NT_ASSERT(FALSE);
    }
}



_IRQL_requires_max_(DISPATCH_LEVEL)
void HfpSrvIndicationCallback(_In_ void* Context, _In_ SCO_INDICATION_CODE Indication, _In_ SCO_INDICATION_PARAMETERS* Parameters)
{
    switch(Indication)
    {
        case ScoIndicationAddReference:
        case ScoIndicationReleaseReference:
            break;

        case ScoIndicationRemoteConnect:
            HfpSrvSendConnectResponse((HFPDEVICE_CONTEXT*)Context, Parameters);
            break;

        case ScoIndicationRemoteDisconnect:
            // We register HfpSrvConnectionIndicationCallback for disconnect
            NT_ASSERT(FALSE);
            break;
    }
}

