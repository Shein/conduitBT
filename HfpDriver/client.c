/*++

Module Name:
    client.c

Abstract:
    Contains Bluetooth client functionality.
    
Environment:
    Kernel mode only
--*/

#include "clisrv.h"
#include "device.h"
#include "client.h"

#define INITGUID
#include "hfppublic.h"

#if defined(EVENT_TRACING)
#include "client.tmh"
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, HfpBthQueryInterfaces)
//#pragma alloc_text (PAGE, HfpRetrieveServerSdpRecord)
#endif


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS HfpBthQueryInterfaces(_In_ HFPDEVICE_CONTEXT* DevCtx)
{
    NTSTATUS status;

    PAGED_CODE();

    status = WdfFdoQueryForInterface(
        DevCtx->Header.Device,
        &GUID_BTHDDI_PROFILE_DRIVER_INTERFACE, 
        (PINTERFACE) (&DevCtx->Header.ProfileDrvInterface),
        sizeof(DevCtx->Header.ProfileDrvInterface), 
        BTHDDI_PROFILE_DRIVER_INTERFACE_VERSION_FOR_QI, 
        NULL
        );
                
    if (!NT_SUCCESS(status))  {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "QueryInterface failed for Interface profile driver interface, version %d, Status code %!STATUS!\n", BTHDDI_PROFILE_DRIVER_INTERFACE_VERSION_FOR_QI, status);
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
			TraceEvents (TRACE_LEVEL_INFORMATION, DBG_CONNECT, "ScoIndicationAddReference\n");
            break;

        case ScoIndicationReleaseReference:
			TraceEvents (TRACE_LEVEL_INFORMATION, DBG_CONNECT, "ScoIndicationReleaseReference\n");
            break;

        case ScoIndicationRemoteConnect:
            // We don't expect connection
			TraceEvents (TRACE_LEVEL_INFORMATION, DBG_CONNECT, "Remote Connect indication: UNEXPECTED\n");
            break;

        case IndicationRemoteDisconnect:
            // This is an indication that server has disconnected. In response we disconnect from our end
			TraceEvents (TRACE_LEVEL_INFORMATION, DBG_CONNECT, "Remote Disconnect indication\n");
            HfpConnectionObjectRemoteDisconnect (connection->DevCtxHdr, connection);
            break;
    }
}



void HfpRemoteConnectCompletion (_In_ WDFREQUEST  Request, _In_ WDFIOTARGET Target, _In_ PWDF_REQUEST_COMPLETION_PARAMS  Params, _In_ WDFCONTEXT  Context)
{
    NTSTATUS status;
    struct _BRB_SCO_OPEN_CHANNEL *brb;    
    HFPDEVICE_CONTEXT* DevCtx;
    HFP_CONNECTION* connection;
    
    DevCtx = GetClientDeviceContext(WdfIoTargetGetDevice(Target));

    status = Params->IoStatus.Status;

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, "Connection completion, status: %X\n", status);

    brb = (struct _BRB_SCO_OPEN_CHANNEL*) Context;

    connection = (HFP_CONNECTION*) brb->Hdr.ClientContext[0];

    // In the client we don't check for ConnectionStateDisconnecting state because only file close generates disconnect 
	// which cannot happen before create completes. And we complete Create only after we process this completion.

    if (NT_SUCCESS(status))
    {
        connection->ChannelHandle   = brb->ChannelHandle;
        connection->RemoteAddress   = brb->BtAddress;
        connection->ConnectionState = ConnectionStateConnected;

        TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, "Connection established to server\n");

        // Set the file context to the connection passed in
		GetFileContext(WdfRequestGetFileObject(Request))->Connection = connection;
        // If such post processing here fails we may disconnect:
        // HfpConnectionObjectRemoteDisconnect (&(DevCtx->Header), connection);
    }
    else {
        connection->ConnectionState = ConnectionStateConnectFailed;
    }

    // Complete the Create request
    WdfRequestComplete(Request, status);
    return;    
}



_IRQL_requires_same_
NTSTATUS HfpOpenRemoteConnection (_In_ HFPDEVICE_CONTEXT* DevCtx, _In_ WDFFILEOBJECT FileObject, _In_ WDFREQUEST Request)
{
    NTSTATUS status;
    WDFOBJECT connectionObject;    
    struct _BRB_SCO_OPEN_CHANNEL *brb = NULL;
    HFP_CONNECTION* connection = NULL;
    //HFP_FILE_CONTEXT* fileCtx = GetFileContext(FileObject); KS - no need, was needed in L2CAP

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, "Connect request\n");

    // Create the connection object that would store information about the open channel
    // Set file object as the parent for this connection object
    status = HfpConnectionObjectCreate (&DevCtx->Header,FileObject/*parent*/, &connectionObject);
    if (!NT_SUCCESS(status))
        goto exit;

    connection = GetConnectionObjectContext(connectionObject);
    connection->ConnectionState = ConnectionStateConnecting;

    // Get the BRB from request context and initialize it as BRB_SCO_OPEN_CHANNEL BRB
    brb = (struct _BRB_SCO_OPEN_CHANNEL *)GetRequestContext(Request);

    DevCtx->Header.ProfileDrvInterface.BthReuseBrb((PBRB)brb, BRB_SCO_OPEN_CHANNEL);
    
    brb->Hdr.ClientContext[0] = connection;

    brb->BtAddress			= DevCtx->ServerBthAddress;
	brb->TransmitBandwidth	= 
	brb->ReceiveBandwidth	= 8000;  // 64Kb/s
	brb->MaxLatency			= 0xF;
	brb->PacketType			= SCO_PKT_ALL;
	brb->ContentFormat		= SCO_VS_IN_CODING_ALAW | SCO_VS_IN_SAMPLE_SIZE_8BIT | SCO_VS_AIR_CODING_FORMAT_CVSD;
	brb->RetransmissionEffort= SCO_RETRANSMISSION_NONE;
    brb->ChannelFlags		= SCO_CF_LINK_SUPPRESS_PIN;
    brb->CallbackFlags		= SCO_CALLBACK_DISCONNECT;	// Get notification about remote disconnect 
    brb->Callback			= &HfpIndicationCallback;
    brb->CallbackContext	= connection;
    brb->ReferenceObject	= (PVOID) WdfDeviceWdmGetDeviceObject(DevCtx->Header.Device);

    status = HfpSharedSendBrbAsync(DevCtx->Header.IoTarget, Request, (PBRB)brb, sizeof(*brb), HfpRemoteConnectCompletion, brb/*Context*/);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_CONNECT, "Sending brb for opening connection failed, returning status code %!STATUS!\n", status);
        goto exit;
    }            

	exit:
    if (!NT_SUCCESS(status)) {
        if (connection) {
            // Set the right state to facilitate debugging
            connection->ConnectionState = ConnectionStateConnectFailed;            
        }
        // In case of failure of this routine we will fail Create which will delete file object 
		// and since connection object is child of the file object, it will be deleted too
		// KS: TODO, since I'm moving this routine call from file creation to DeviceIoControl, this situation should be cared 
    }
    
    return status;
}


