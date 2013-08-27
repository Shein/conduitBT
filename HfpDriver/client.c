/*++

Module Name:
    client.c

Abstract:
    Contains Bluetooth client functionality.
    
Environment:
    Kernel mode only
--*/

#include "driver.h"
#include "device.h"
#include "connection.h"
#include "clisrv.h"
#include "client.h"

#define INITGUID
#include "hfppublic.h"

#if defined(EVENT_TRACING)
#include "client.tmh"
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, HfpBthQueryInterfaces)
#endif


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS HfpBthQueryInterfaces(_In_ HFPDEVICE_CONTEXT* devCtx)
{
    NTSTATUS status;

    PAGED_CODE();

    status = WdfFdoQueryForInterface(
        devCtx->Device,
        &GUID_BTHDDI_PROFILE_DRIVER_INTERFACE, 
        (PINTERFACE) (&devCtx->ProfileDrvInterface),
        sizeof(devCtx->ProfileDrvInterface), 
        BTHDDI_PROFILE_DRIVER_INTERFACE_VERSION_FOR_QI, 
        NULL
        );
                
    if (!NT_SUCCESS(status))  {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "QueryInterface failed, version %d, Status %X", BTHDDI_PROFILE_DRIVER_INTERFACE_VERSION_FOR_QI, status);
        goto exit;
    }

	exit:
    return status;
}


_IRQL_requires_max_(DISPATCH_LEVEL)
void HfpIndicationCallback (_In_ PVOID Context, _In_ SCO_INDICATION_CODE Indication, _In_ PSCO_INDICATION_PARAMETERS Parameters)
{
    HFP_CONNECTION* connection = (HFP_CONNECTION*) Context;

    UNREFERENCED_PARAMETER(Parameters);

    // Only supporting connect and disconnect
    switch (Indication)
    {
        // We don't add/release reference to anything because our connection is scoped within file object lifetime
        case ScoIndicationAddReference:
			TraceEvents (TRACE_LEVEL_INFORMATION, DBG_CONNECT, "HfpIndicationCallback - AddReference");
            break;

        case ScoIndicationReleaseReference:
			TraceEvents (TRACE_LEVEL_INFORMATION, DBG_CONNECT, "HfpIndicationCallback - ReleaseReference");
            break;

        case ScoIndicationRemoteConnect:
            // We don't expect connection
			TraceEvents (TRACE_LEVEL_INFORMATION, DBG_CONNECT, "HfpIndicationCallback - Connect (UNEXPECTED)");
            break;

        case ScoIndicationRemoteDisconnect:
            // This is an indication that server has disconnected. In response we disconnect from our end
			TraceEvents (TRACE_LEVEL_INFORMATION, DBG_CONNECT, "HfpIndicationCallback - Disconnect");
			if (connection->DevCtx->KevScoDisconnect)
				KeSetEvent(connection->DevCtx->KevScoDisconnect,0,FALSE);
            HfpConnectionObjectRemoteDisconnect (connection->DevCtx, connection);
            break;
    }
}



void HfpRemoteConnectCompletion (_In_ WDFREQUEST Request, _In_ WDFIOTARGET Target, _In_ PWDF_REQUEST_COMPLETION_PARAMS Params, _In_ WDFCONTEXT Context)
{
    NTSTATUS					  status	 = Params->IoStatus.Status;
    struct _BRB_SCO_OPEN_CHANNEL* brb		 = (struct _BRB_SCO_OPEN_CHANNEL*) Context;
    HFP_CONNECTION*				  connection = (HFP_CONNECTION*) brb->Hdr.ClientContext[0];
    
    UNREFERENCED_PARAMETER(Target);
    
    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, "Outgoing Connection completion, Status %X", status);

    // In the client we don't check for ConnectionStateDisconnecting state because only file close generates disconnect 
	// which cannot happen before create completes. And we complete Create only after we process this completion.

    if (NT_SUCCESS(status))
    {
        connection->ChannelHandle   = brb->ChannelHandle;
        connection->RemoteAddress   = brb->BtAddress;
        connection->ConnectionState = ConnectionStateConnected;

        // Set the file context to the connection passed in
		GetFileContext(WdfRequestGetFileObject(Request))->Connection = connection;

        // If such post processing here fails we may disconnect:
        // HfpConnectionObjectRemoteDisconnect (devCtx, connection);
    }
    else {
        connection->ConnectionState = ConnectionStateConnectFailed;
    }

    // Complete the Create request
    WdfRequestComplete(Request, status);
    return;    
}



_IRQL_requires_same_
NTSTATUS HfpOpenRemoteConnection (_In_ HFPDEVICE_CONTEXT* devCtx, _In_ WDFFILEOBJECT fileObject, _In_ WDFREQUEST Request)
{
    NTSTATUS						status;
    WDFOBJECT						connectionObject;    
    HFP_CONNECTION*					connection = 0;
    struct _BRB_SCO_OPEN_CHANNEL*	brb;


    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, "Outgoing SCO connect request");

    // Create the connection object that would store information about the open channel
    // Set file object as the parent for this connection object
    status = HfpConnectionObjectCreate (devCtx, fileObject/*parent*/, &connectionObject);
    if (!NT_SUCCESS(status))
        goto exit;

    connection = GetConnectionObjectContext(connectionObject);
    connection->ConnectionState = ConnectionStateConnecting;

    // Get the BRB from request context and initialize it as BRB_SCO_OPEN_CHANNEL
    brb = (struct _BRB_SCO_OPEN_CHANNEL*) GetRequestContext(Request);

    devCtx->ProfileDrvInterface.BthReuseBrb((PBRB)brb, BRB_SCO_OPEN_CHANNEL);
    
    brb->Hdr.ClientContext[0] = connection;

    brb->BtAddress			= devCtx->RemoteBthAddress;
	brb->TransmitBandwidth	= 
	brb->ReceiveBandwidth	= 8000;  // 64Kb/s
	brb->MaxLatency			= 0xF;
	brb->PacketType			= SCO_PKT_ALL;
	brb->ContentFormat		= SCO_VS_IN_CODING_LINEAR | SCO_VS_IN_SAMPLE_SIZE_16BIT | SCO_VS_AIR_CODING_FORMAT_CVSD;
	brb->RetransmissionEffort= SCO_RETRANSMISSION_NONE;
    brb->ChannelFlags		= SCO_CF_LINK_SUPPRESS_PIN;
    brb->CallbackFlags		= SCO_CALLBACK_DISCONNECT;	// Get notification about remote disconnect 
    brb->Callback			= &HfpIndicationCallback;
    brb->CallbackContext	= connection;
    brb->ReferenceObject	= (PVOID) WdfDeviceWdmGetDeviceObject(devCtx->Device);

    status = HfpSharedSendBrbAsync(devCtx->IoTarget, Request, (PBRB)brb, sizeof(*brb), HfpRemoteConnectCompletion, brb/*Context*/);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_CONNECT, "Sending BRB for opening connection failed, Status %X", status);
        goto exit;
    }            

	exit:
    if (!NT_SUCCESS(status)) {
        if (connection)
            connection->ConnectionState = ConnectionStateConnectFailed;    // to facilitate debugging

        // In case of failure of this routine we will fail Create which will delete file object 
		// and since connection object is child of the file object, it will be deleted too
		// KS TODO: since I'm moving this routine call from the file creation to DeviceIoControl, this situation 
		// should be cared: despite it is still fileObject's child, now it doesn't fail CreateFile, and this object
		// will be remained till next CloseFile.
    }
    
    return status;
}

