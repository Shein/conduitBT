/*++
  Module Name:
    Queue.c

  Abstract:
    Read/write functionality for bth HFP

  Environment:
    Kernel mode only
--*/

#include "driver.h"
#include "device.h"
#include "connection.h"
#include "queue.h"
#include "client.h"
#include "server.h"

#if defined(EVENT_TRACING)
#include "queue.tmh"
#endif


void HfpReadWriteCompletion (_In_ WDFREQUEST Request, _In_ WDFIOTARGET  Target, _In_ PWDF_REQUEST_COMPLETION_PARAMS Params, _In_ WDFCONTEXT  Context)
{
    struct _BRB_SCO_TRANSFER *brb = (struct _BRB_SCO_TRANSFER*) Context;
    size_t bufsize;

    UNREFERENCED_PARAMETER(Target);
    NT_ASSERT (brb);

    bufsize = brb->BufferSize;  // bytes read/written

    if (Params->IoStatus.Status) {
		// Print only when error happened 
		TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, "I/O completion, Status %X, size = %d", Params->IoStatus.Status, bufsize);
	}

    // Complete the request
    WdfRequestCompleteWithInformation(Request, Params->IoStatus.Status, bufsize);
}



void HfpEvtQueueIoWrite (_In_ WDFQUEUE Queue, _In_ WDFREQUEST Request, _In_ size_t Length)
{
    NTSTATUS			status;
    HFPDEVICE_CONTEXT*	devCtx;
    WDFMEMORY			memory;
    struct _BRB_SCO_TRANSFER * brb = NULL;    
    HFP_CONNECTION*		connection;
    
    UNREFERENCED_PARAMETER(Length);

    //TraceEvents(TRACE_LEVEL_INFORMATION, DBG_WRITE, "IoWrite request %d bytes", Length);

    connection = GetFileContext(WdfRequestGetFileObject(Request))->Connection;
	if (!connection) {
		status = STATUS_INVALID_DEVICE_REQUEST;
		goto exit;
	}
    devCtx = GetClientDeviceContext(WdfIoQueueGetDevice(Queue));
    status = WdfRequestRetrieveInputMemory (Request, &memory);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE, "WdfRequestRetrieveInputMemory failed, request 0x%p, Status %X", Request, status);
        goto exit;        
    }

    // Get the BRB from request context and initialize it as BRB_SCO_TRANSFER BRB
    brb = (struct _BRB_SCO_TRANSFER*) GetRequestContext(Request);
    devCtx->ProfileDrvInterface.BthReuseBrb ((PBRB)brb, BRB_SCO_TRANSFER);

    // Format the Write request for SCO OUT transfer. This routine allocates a BRB which is returned to us in brb parameter.
    // This BRB is freed by the completion routine is we send the request successfully, else it is freed by this routine.
    status = HfpConnectionObjectFormatRequestForScoTransfer (connection, Request, &brb, memory, SCO_TRANSFER_DIRECTION_OUT);

    if (!NT_SUCCESS(status))
        goto exit;

    // Set a CompletionRoutine callback function.
    WdfRequestSetCompletionRoutine(Request, HfpReadWriteCompletion, brb);

    // Send the request down the stack
    if (!WdfRequestSend(Request, devCtx->IoTarget, NULL))  {
        status = WdfRequestGetStatus(Request);
        TraceEvents(TRACE_LEVEL_ERROR, DBG_UTIL, "Request send failed for request 0x%p, BRB %p, Status %X", Request, brb, status);
        goto exit;
    }    

	exit:
    if (!NT_SUCCESS(status))
        WdfRequestComplete(Request, status);
}



void HfpEvtQueueIoRead (_In_ WDFQUEUE  Queue, _In_ WDFREQUEST  Request, _In_ size_t Length)
{
    NTSTATUS			status;
    HFPDEVICE_CONTEXT*	devCtx;
    WDFMEMORY			memory;
    struct _BRB_SCO_TRANSFER* brb = NULL;    
    HFP_CONNECTION*		connection;
    
    UNREFERENCED_PARAMETER(Length);

    // TraceEvents(TRACE_LEVEL_INFORMATION, DBG_WRITE, "IoRead request %d bytes", Length);

    devCtx	   = GetClientDeviceContext(WdfIoQueueGetDevice(Queue));
    connection = GetFileContext(WdfRequestGetFileObject(Request))->Connection;
	if (!connection) {
		status = STATUS_INVALID_DEVICE_REQUEST;
		goto exit;
	}

    status = WdfRequestRetrieveOutputMemory (Request, &memory);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE, "WdfRequestRetrieveInputMemory failed, request 0x%p, Status %X", Request, status);
        goto exit;        
    }

    // Get the BRB from request context and initialize it as BRB_SCO_TRANSFER BRB
    brb = (struct _BRB_SCO_TRANSFER*) GetRequestContext(Request);
    devCtx->ProfileDrvInterface.BthReuseBrb ((PBRB)brb,  BRB_SCO_TRANSFER);

    // Format the Read request for SCO IN transfer. This routine allocates a BRB which is returned to us in brb parameter.
    // This BRB is freed by the completion routine is we send the request successfully, else it is freed by this routine.
    status = HfpConnectionObjectFormatRequestForScoTransfer (connection, Request, &brb, memory, SCO_TRANSFER_DIRECTION_IN);

    if (!NT_SUCCESS(status))
        goto exit;

    // Set a CompletionRoutine callback function.
    WdfRequestSetCompletionRoutine (Request, HfpReadWriteCompletion, brb);

    // Send the request down the stack
    if (!WdfRequestSend (Request, devCtx->IoTarget, NULL))  {
        status = WdfRequestGetStatus(Request);
        TraceEvents(TRACE_LEVEL_ERROR, DBG_UTIL, "Request send failed for request 0x%p, Brb 0x%p, Status %X", Request, brb, status);
        goto exit;
    }

	exit:
    if (!NT_SUCCESS(status))
        WdfRequestComplete(Request, status);
}



void HfpEvtQueueIoStop (_In_ WDFQUEUE  Queue, _In_ WDFREQUEST  Request, _In_ ULONG  ActionFlags)
{
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(ActionFlags);

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_WRITE, "HfpEvtQueueIoStop");
    WdfRequestCancelSentRequest(Request);
}



void HfpEvtQueueIoDeviceControl (WDFQUEUE Queue, WDFREQUEST Request, size_t OutBufferLen, size_t InBufferLen, ULONG IoControlCode)
{
    NTSTATUS			status;
    PVOID				inbuf;
    size_t				size;
	HFP_CONNECTION*		connection;
    HFPDEVICE_CONTEXT*	devCtx = GetClientDeviceContext(WdfIoQueueGetDevice(Queue));

	UNREFERENCED_PARAMETER(InBufferLen);
	UNREFERENCED_PARAMETER(OutBufferLen);

    switch (IoControlCode) 
	{
        case IOCTL_HFP_REG_SERVER:
			TraceEvents(TRACE_LEVEL_INFORMATION, DBG_WRITE, "HfpEvtQueueIoDeviceControl: REG_SERVER");

			// For buffered IOCTLs WdfRequestRetrieveInputBuffer & WdfRequestRetrieveOutputBuffer return the same buffer:
			// pointer (Irp->AssociatedIrp.SystemBuffer)
			status = WdfRequestRetrieveInputBuffer(Request, 0, &inbuf, &size);
			if (!NT_SUCCESS(status))
				break;

			// Also may be checked here: NT_ASSERT(size == InBufferLen)
			// We will check that the supplied buffer is our struct
			if (size != sizeof(HFP_REG_SERVER)) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			// HfpSrvRegisterScoServer uses another Request (our devCtx->Request), but this is also ok
			status = HfpSrvRegisterScoServer (devCtx, (HFP_REG_SERVER*)inbuf);
            break;

        case IOCTL_HFP_UNREG_SERVER:
			TraceEvents(TRACE_LEVEL_INFORMATION, DBG_WRITE, "HfpEvtQueueIoDeviceControl: UNREG_SERVER");
			if (!devCtx->ScoServerHandle) {
				status = STATUS_INVALID_DEVICE_REQUEST;
				break;
			}
			status = HfpSrvUnregisterScoServer (devCtx, 0);
            break;

        case IOCTL_HFP_OPEN_SCO:
			TraceEvents(TRACE_LEVEL_INFORMATION, DBG_WRITE, "HfpEvtQueueIoDeviceControl: OPEN_SCO");
			status = HfpOpenRemoteConnection(devCtx, WdfRequestGetFileObject(Request), Request);
			// if it succeeds RemoteConnectCompletion will complete the request; so return here
			if (NT_SUCCESS(status))
				return;
            break;

        case IOCTL_HFP_CLOSE_SCO:
			TraceEvents(TRACE_LEVEL_INFORMATION, DBG_WRITE, "HfpEvtQueueIoDeviceControl: CLOSE_SCO");
			connection = GetFileContext(WdfRequestGetFileObject(Request))->Connection;
			if (connection)
				HfpConnectionObjectRemoteDisconnectSynchronously (devCtx, connection);
            status = STATUS_SUCCESS;
            break;

        case IOCTL_HFP_INCOMING_READINESS:
			status = WdfRequestRetrieveInputBuffer(Request, 0, &inbuf, &size);
			if (!NT_SUCCESS(status))
				break;

			// Also may be checked here: NT_ASSERT(size == InBufferLen)
			// We will check that the supplied buffer is our struct
			if (size != sizeof(BOOLEAN)) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}

			TraceEvents(TRACE_LEVEL_INFORMATION, DBG_WRITE, "HfpEvtQueueIoDeviceControl: INCOMING_READINESS = %d", *(BOOLEAN*)inbuf);
			devCtx->ConnectReadiness = *(BOOLEAN*)inbuf;
            status = STATUS_SUCCESS;
            break;

		default:
            status = STATUS_INVALID_PARAMETER;
    }

    WdfRequestComplete (Request, status);
    return;
}

