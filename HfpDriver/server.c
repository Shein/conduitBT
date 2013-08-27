/*++

Module Name:
    Server.c

Abstract:
    Contains Bluetooth server functionality.
    
Environment:
    Kernel mode only
--*/


#include "driver.h"
#include "device.h"
#include "connection.h"
#include "clisrv.h"
#include "server.h"
#include "sdp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, HfpSrvPublishSdpRecord)
#pragma alloc_text (PAGE, HfpSrvRemoveSdpRecord)
#endif


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
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "QueryInterface failed for Interface SDP parse, Status %X", status);
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
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "QueryInterface failed for Interface SDP node, Status %X", status);
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
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "CreateSdpRecord failed, Status %X", status);
        goto exit;
    }
    
	// -----------------------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------------------

    PAGED_CODE();

    WDF_REQUEST_REUSE_PARAMS_INIT(&ReuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_NOT_SUPPORTED);
    statusReuse = WdfRequestReuse(devCtx->Request, &ReuseParams);    
    NT_ASSERT(NT_SUCCESS(statusReuse));
    UNREFERENCED_PARAMETER(statusReuse);

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER (&inMemDesc, sdpRecordStream, sdpRecordLength);
    RtlZeroMemory( &sdpRecordHandle, sizeof(HANDLE_SDP) );
    
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER (&outMemDesc, &sdpRecordHandle, sizeof(HANDLE_SDP));

    status = WdfIoTargetSendIoctlSynchronously(
        devCtx->IoTarget,
        devCtx->Request,
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
    statusReuse =  WdfRequestReuse(devCtx->Request, &ReuseParams);    
    NT_ASSERT(NT_SUCCESS(statusReuse));
    UNREFERENCED_PARAMETER(statusReuse);

    sdpRecordHandle = devCtx->SdpRecordHandle;

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER (&inMemDesc, &sdpRecordHandle, sizeof(HANDLE_SDP));
    
    status = WdfIoTargetSendIoctlSynchronously(
        devCtx->IoTarget,
        devCtx->Request,
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


NTSTATUS HfpSrvRegisterScoServer (_In_ HFPDEVICE_CONTEXT* devCtx, _In_ HFP_REG_SERVER* regparams)
{
	NTSTATUS status = STATUS_SUCCESS;
	struct _BRB_SCO_REGISTER_SERVER* brb;
	PKEVENT  kev1, kev2;


	// First to create Events and only then to register Server

	if (devCtx->ScoServerHandle)
		HfpSrvUnregisterScoServer (devCtx, regparams->DestAddr);

	status = ObReferenceObjectByHandle(regparams->EvHandleScoConnect, EVENT_MODIFY_STATE, *ExEventObjectType, UserMode, (PVOID*)&kev1, 0);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "ObReferenceObjectByHandle failed, Status %X", status);
		goto exit;
	}

	status = ObReferenceObjectByHandle(regparams->EvHandleScoDisconnect, EVENT_MODIFY_STATE, *ExEventObjectType, UserMode, (PVOID*)&kev2, 0);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "ObReferenceObjectByHandle failed, Status %X", status);
		goto exit;
	}

	devCtx->KevScoConnect	 = kev1;
	devCtx->KevScoDisconnect = kev2;
	devCtx->ConnectReadiness = regparams->ConnectReadiness;

	if (devCtx->ScoServerHandle) {
		// See the description at HfpSrvUnregisterScoServer prototype
		goto exit_with_reg;
	}

	// Registers SCO server
	devCtx->ProfileDrvInterface.BthReuseBrb(&(devCtx->RegisterUnregisterBrb), BRB_SCO_REGISTER_SERVER);

	brb = (struct _BRB_SCO_REGISTER_SERVER*) &(devCtx->RegisterUnregisterBrb);

	// Format BRB
	brb->BtAddress					= regparams->DestAddr;		// cannot be BTH_ADDR_NULL: SCO doesn't support it, opposite to L2CAP
	brb->IndicationCallback			= &HfpSrvIndicationCallback;
	brb->IndicationCallbackContext	= devCtx;
	brb->IndicationFlags			= SCO_INDICATION_SCO_REQUEST;
	brb->ReferenceObject			= WdfDeviceWdmGetDeviceObject(devCtx->Device);

	status = HfpSharedSendBrbSynchronously (devCtx->IoTarget, devCtx->Request, (PBRB)brb, sizeof(*brb));

	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "BRB_SCO_REGISTER_SERVER failed, Status %X", status);
		goto exit;
	}

	// Store server handles
	devCtx->RemoteBthAddress = regparams->DestAddr;
	devCtx->ScoServerHandle  = brb->ServerHandle;

	exit_with_reg:
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, "BRB_SCO_REGISTER_SERVER completed, handle = %X, RemoteBthAddress = %llX", devCtx->ScoServerHandle, devCtx->RemoteBthAddress);

	exit:
	return status;
}


NTSTATUS HfpSrvUnregisterScoServer (_In_ HFPDEVICE_CONTEXT* devCtx, _Inout_ UINT64 destaddr)
{
	NTSTATUS status;
	struct _BRB_SCO_UNREGISTER_SERVER *brb;


	// First to unregister Server and only then to delete Events 

	if (destaddr  &&  (destaddr == devCtx->RemoteBthAddress)) {
		// See the description at HfpSrvUnregisterScoServer prototype
		status = STATUS_SUCCESS;
		goto skip_unregistration;
	}

	devCtx->ProfileDrvInterface.BthReuseBrb (&(devCtx->RegisterUnregisterBrb), BRB_SCO_UNREGISTER_SERVER);

	brb = (struct _BRB_SCO_UNREGISTER_SERVER*) &(devCtx->RegisterUnregisterBrb);

	brb->BtAddress	  = devCtx->RemoteBthAddress;
	brb->ServerHandle = devCtx->ScoServerHandle;

	status = HfpSharedSendBrbSynchronously (devCtx->IoTarget, devCtx->Request, (PBRB)brb, sizeof(*brb));

	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "BRB_SCO_UNREGISTER_SERVER failed, Status=%X", status);
		// Send does not fail for resource reasons
		NT_ASSERT(FALSE);
		goto exit;
	}

	devCtx->ScoServerHandle  = 0;
	devCtx->RemoteBthAddress = 0;

	skip_unregistration:
	ObDereferenceObject (devCtx->KevScoConnect);
	ObDereferenceObject (devCtx->KevScoDisconnect);

	devCtx->KevScoConnect    = 0;
	devCtx->KevScoDisconnect = 0;

	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, "BRB_SCO_UNREGISTER_SERVER completed");
	exit:
	return status;
}



void HfpSrvRemoteConnectCompletion (_In_ WDFREQUEST Request, _In_ WDFIOTARGET Target, _In_ PWDF_REQUEST_COMPLETION_PARAMS Params, _In_ WDFCONTEXT Context)
{
    NTSTATUS					  status		   = Params->IoStatus.Status;
    struct _BRB_SCO_OPEN_CHANNEL* brb			   = (struct _BRB_SCO_OPEN_CHANNEL*) Context;
    WDFOBJECT					  connectionObject = (WDFOBJECT) brb->Hdr.ClientContext[0];	// We receive connection object as the context in the BRB
    HFP_CONNECTION*				  connection	   = GetConnectionObjectContext(connectionObject);

    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Request); //we reuse the request, hence it is not needed here
   
    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, "Incoming Connection completion, Status %X", status);

    if (NT_SUCCESS(status))
    {
        connection->ChannelHandle = brb->ChannelHandle;
        connection->RemoteAddress = brb->BtAddress;

        // Check if we already received a disconnect request and if so disconnect
        WdfSpinLockAcquire(connection->ConnectionLock);

        if (connection->ConnectionState == ConnectionStateDisconnecting) 
        {
            TraceEvents(TRACE_LEVEL_WARNING, DBG_CONNECT, "HfpSrvRemoteConnectCompletion: disconnect while connecting");
            // We allow transition to disconnected state only from connected state. If we are in disconnection state 
			// this means that we were waiting for connect to complete before we can send disconnect down.
            // Set the state to Connected and call HfpConnectionObjectRemoteDisconnect
            connection->ConnectionState = ConnectionStateConnected;
            WdfSpinLockRelease(connection->ConnectionLock);
            HfpConnectionObjectRemoteDisconnect(connection->DevCtx, connection);
        }
        else {
            connection->ConnectionState = ConnectionStateConnected;
            WdfSpinLockRelease(connection->ConnectionLock);

			// Set the file context to the connection passed in
			// KS: Because of we are opening incoming connection, we do not have the associated File object set 
			// for it (i think), so this thing, like in outgoing connection creation, can not be used: 
			// GetFileContext(WdfRequestGetFileObject(Request))->Connection = connection;
			// So, take our global file object from the Device context (TODO: this code must be locked)

			NT_ASSERT (connection->FileObject);
			GetFileContext(connection->FileObject)->Connection = connection;

			if (connection->DevCtx->ConnectReadiness)
			{
				// Notify User mode app about new connection
				if (connection->DevCtx->KevScoConnect)
					KeSetEvent(connection->DevCtx->KevScoConnect,0,FALSE);
			}
			else {
				TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, "Not ready, rejecting SCO");
				HfpConnectionObjectRemoteDisconnect(connection->DevCtx, connection);
			}
        }
    }
    else
    {
        BOOLEAN discon = FALSE;

        WdfSpinLockAcquire(connection->ConnectionLock);
        if (connection->ConnectionState == ConnectionStateDisconnecting) 
            discon = TRUE;

        connection->ConnectionState = ConnectionStateConnectFailed;
        WdfSpinLockRelease(connection->ConnectionLock);

        if (discon)
            KeSetEvent (&connection->DisconnectEvent,0,FALSE);
    }

    if (!NT_SUCCESS(status))
        WdfObjectDelete(connectionObject);        

    return;
}



_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS HfpSrvSendConnectResponse(_In_ HFPDEVICE_CONTEXT* devCtx, _In_ SCO_INDICATION_PARAMETERS* ConnectParams)
{
    NTSTATUS						status, statusReuse;
    WDF_REQUEST_REUSE_PARAMS		reuseParams;
    WDFOBJECT						connectionObject = 0;
	HFP_CONNECTION*					connection = 0;
	HFP_FILE_CONTEXT*				fileCtx;
	struct _BRB_SCO_OPEN_CHANNEL*	brb;


    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, "Incoming SCO connect request, LinkType = %X", ConnectParams->Parameters.Connect.Request.LinkType);
	if (!devCtx->FileObject)	{
		TraceEvents(TRACE_LEVEL_WARNING, DBG_CONNECT, "Application not ready");
		status = STATUS_SUCCESS;
		goto exit;
	}

	fileCtx = GetFileContext(devCtx->FileObject);
	if (fileCtx->Connection) {
		TraceEvents(TRACE_LEVEL_WARNING, DBG_CONNECT, "Only one SCO connection supported, aborting");
		status = STATUS_SUCCESS;
		goto exit;
	}

    // We create the connection object as the first step so that if we receive 
    // remove before connect response is completed we can wait for connection and disconnect.
    status = HfpConnectionObjectCreate(devCtx, devCtx->FileObject, &connectionObject);
    if (!NT_SUCCESS(status))
        goto exit;

    connection = GetConnectionObjectContext(connectionObject);
    connection->ConnectionState = ConnectionStateConnecting;
	fileCtx->Connection = connection;

    WDF_REQUEST_REUSE_PARAMS_INIT(&reuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_NOT_SUPPORTED);
    statusReuse = WdfRequestReuse(connection->ConnectDisconnectRequest, &reuseParams);
    NT_ASSERT(NT_SUCCESS(statusReuse));
    UNREFERENCED_PARAMETER(statusReuse);
    
    brb = (struct _BRB_SCO_OPEN_CHANNEL*) &(connection->ConnectDisconnectBrb);
    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, "&HFP_CONNECTION = %X, &BRB_SCO_OPEN_CHANNEL = %X", connection, brb);
    devCtx->ProfileDrvInterface.BthReuseBrb((PBRB)brb, BRB_SCO_OPEN_CHANNEL_RESPONSE);

    brb->Hdr.ClientContext[0]	= connectionObject;
    brb->BtAddress				= ConnectParams->BtAddress;
    brb->ChannelHandle			= ConnectParams->ConnectionHandle;
    brb->Response				= SCO_CONNECT_RSP_RESPONSE_SUCCESS;
	brb->TransmitBandwidth		= 
	brb->ReceiveBandwidth		= 8000;  // 64Kb/s
	brb->MaxLatency				= 0xF;
	brb->PacketType				= SCO_PKT_ALL;
	brb->ContentFormat			= SCO_VS_IN_CODING_LINEAR | SCO_VS_IN_SAMPLE_SIZE_16BIT | SCO_VS_AIR_CODING_FORMAT_CVSD;
	brb->RetransmissionEffort	= SCO_RETRANSMISSION_NONE;
    brb->ChannelFlags			= SCO_CF_LINK_SUPPRESS_PIN;
    brb->CallbackFlags			= CALLBACK_DISCONNECT;
    brb->Callback				= &HfpSrvIndicationCallback;
    brb->CallbackContext		= connectionObject;
    brb->ReferenceObject		= (void*) WdfDeviceWdmGetDeviceObject(devCtx->Device);

    status = HfpSharedSendBrbAsync(devCtx->IoTarget, connection->ConnectDisconnectRequest, (PBRB)brb, sizeof(*brb), HfpSrvRemoteConnectCompletion, brb);

    if (!NT_SUCCESS(status))
        TraceEvents(TRACE_LEVEL_ERROR, DBG_CONNECT, "HfpSharedSendBrbAsync failed, Status = %X", status);
    
	exit:
    if (!NT_SUCCESS(status) && connectionObject) {
        // If we failed to connect remove connection from list and delete the object.
        // Connection should not be NULL, if connectionObject is not NULL since first thing we do 
		// after creating connectionObject is to get context which gives us connection
        NT_ASSERT (connection);

        if (connection)
            connection->ConnectionState = ConnectionStateConnectFailed;

        WdfObjectDelete(connectionObject);
    }
    return status;
}



_IRQL_requires_max_(DISPATCH_LEVEL)
void HfpSrvDisconnectConnection (_In_ HFP_CONNECTION * connection)
{
    if (HfpConnectionObjectRemoteDisconnect(connection->DevCtx, connection))
        WdfObjectDelete (WdfObjectContextGetObject(connection));
    // else connection is already disconnected
}



_IRQL_requires_max_(DISPATCH_LEVEL)
void HfpSrvIndicationCallback(_In_ void* Context, _In_ SCO_INDICATION_CODE Indication, _In_ SCO_INDICATION_PARAMETERS* Parameters)
{
	HFP_CONNECTION * connection;

    switch(Indication)
    {
        case ScoIndicationAddReference:
			TraceEvents (TRACE_LEVEL_INFORMATION, DBG_CONNECT, "HfpSrvIndicationCallback - AddReference");
            break;

        case ScoIndicationReleaseReference:
			TraceEvents (TRACE_LEVEL_INFORMATION, DBG_CONNECT, "HfpSrvIndicationCallback - ReleaseReference");
            break;

        case ScoIndicationRemoteConnect:
			TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, "HfpSrvIndicationCallback - Connect");
            HfpSrvSendConnectResponse((HFPDEVICE_CONTEXT*)Context, Parameters);
            break;

        case ScoIndicationRemoteDisconnect:
			TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, "HfpSrvIndicationCallback - Disconnect");
            connection = GetConnectionObjectContext((WDFOBJECT)Context /*connectionObject*/);
			if (connection->DevCtx->KevScoDisconnect)
				KeSetEvent(connection->DevCtx->KevScoDisconnect,0,FALSE);
            HfpSrvDisconnectConnection(connection);
            break;
    }
}

