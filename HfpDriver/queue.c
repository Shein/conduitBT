/*++
  Module Name:
    Queue.c

  Abstract:
    Read/write functionality for bth HFP

  Environment:
    Kernel mode only
--*/

#include "device.h"
#include "queue.h"
#include "clisrv.h"
#include "client.h"

#if defined(EVENT_TRACING)
#include "queue.tmh"
#endif


void HfpReadWriteCompletion (_In_ WDFREQUEST Request, _In_ WDFIOTARGET  Target, _In_ PWDF_REQUEST_COMPLETION_PARAMS Params, _In_ WDFCONTEXT  Context)
{
    struct _BRB_SCO_TRANSFER *brb;    
    size_t information;

    UNREFERENCED_PARAMETER(Target);
    
    brb = (struct _BRB_SCO_TRANSFER*) Context;

    NT_ASSERT (brb);

    // Bytes read/written are contained in Brb->BufferSize
    information = brb->BufferSize;

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, "I/O completion, status: %X, size = %d\n", Params->IoStatus.Status, information);

    // Complete the request
    WdfRequestCompleteWithInformation(Request, Params->IoStatus.Status, information);
}



void HfpEvtQueueIoWrite (_In_ WDFQUEUE Queue, _In_ WDFREQUEST Request, _In_ size_t Length)
{
    NTSTATUS			status;
    HFPDEVICE_CONTEXT*	devCtx;
    WDFMEMORY			memory;
    struct _BRB_SCO_TRANSFER * brb = NULL;    
    HFP_CONNECTION*		connection;
    
    UNREFERENCED_PARAMETER(Length);

    //TraceEvents(TRACE_LEVEL_INFORMATION, DBG_WRITE, "-->HfpEvtQueueIoWrite\n");
    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_WRITE, "IoWrite request %d bytes\n", Length);

    connection = GetFileContext(WdfRequestGetFileObject(Request))->Connection;
	// KS: TODO, check connection here, and in the Read func, for null
    devCtx = GetClientDeviceContext(WdfIoQueueGetDevice(Queue));
    status = WdfRequestRetrieveInputMemory (Request, &memory);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE, "WdfRequestRetrieveInputMemory failed, request 0x%p, Status code %d\n", Request, status);
        goto exit;        
    }

    // Get the BRB from request context and initialize it as BRB_SCO_TRANSFER BRB
    brb = (struct _BRB_SCO_TRANSFER*) GetRequestContext(Request);
    devCtx->Header.ProfileDrvInterface.BthReuseBrb ((PBRB)brb, BRB_SCO_TRANSFER);

    // Format the Write request for SCO OUT transfer. This routine allocates a BRB which is returned to us in brb parameter.
    // This BRB is freed by the completion routine is we send the request successfully, else it is freed by this routine.
    status = HfpConnectionObjectFormatRequestForScoTransfer (connection, Request, &brb, memory, SCO_TRANSFER_DIRECTION_OUT);

    if (!NT_SUCCESS(status))
        goto exit;

    // Set a CompletionRoutine callback function.
    WdfRequestSetCompletionRoutine(Request, HfpReadWriteCompletion, brb);

    // Send the request down the stack
    if (!WdfRequestSend(Request, devCtx->Header.IoTarget, NULL))  {
        status = WdfRequestGetStatus(Request);
        TraceEvents(TRACE_LEVEL_ERROR, DBG_UTIL, "Request send failed for request 0x%p, Brb 0x%p, Status code %d\n", Request, brb, status);
        goto exit;
    }    

	exit:
    if (!NT_SUCCESS(status))
        WdfRequestComplete(Request, status);

    //TraceEvents(TRACE_LEVEL_INFORMATION, DBG_WRITE, "<--HfpEvtQueueIoWrite\n");
}



void HfpEvtQueueIoRead (_In_ WDFQUEUE  Queue, _In_ WDFREQUEST  Request, _In_ size_t Length)
{
    NTSTATUS			status;
    HFPDEVICE_CONTEXT*	devCtx;
    WDFMEMORY			memory;
    struct _BRB_SCO_TRANSFER* brb = NULL;    
    HFP_CONNECTION*		connection;
    
    UNREFERENCED_PARAMETER(Length);

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_WRITE, "IoRead request %d bytes\n", Length);

    connection = GetFileContext(WdfRequestGetFileObject(Request))->Connection;
    devCtx	   = GetClientDeviceContext(WdfIoQueueGetDevice(Queue));

    status = WdfRequestRetrieveOutputMemory (Request, &memory);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_WRITE, "WdfRequestRetrieveInputMemory failed, request 0x%p, Status code %d\n", Request, status);
        goto exit;        
    }

    // Get the BRB from request context and initialize it as BRB_SCO_TRANSFER BRB
    brb = (struct _BRB_SCO_TRANSFER*) GetRequestContext(Request);
    devCtx->Header.ProfileDrvInterface.BthReuseBrb ((PBRB)brb,  BRB_SCO_TRANSFER);

    // Format the Read request for SCO IN transfer. This routine allocates a BRB which is returned to us in brb parameter.
    // This BRB is freed by the completion routine is we send the request successfully, else it is freed by this routine.
    status = HfpConnectionObjectFormatRequestForScoTransfer (connection, Request, &brb, memory, SCO_TRANSFER_DIRECTION_IN);

    if (!NT_SUCCESS(status))
        goto exit;

    // Set a CompletionRoutine callback function.
    WdfRequestSetCompletionRoutine (Request, HfpReadWriteCompletion, brb);

    // Send the request down the stack
    if (!WdfRequestSend (Request, devCtx->Header.IoTarget, NULL))  {
        status = WdfRequestGetStatus(Request);
        TraceEvents(TRACE_LEVEL_ERROR, DBG_UTIL, "Request send failed for request 0x%p, Brb 0x%p, Status code %d\n", Request, brb, status);
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

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_WRITE, "HfpEvtQueueIoStop\n");
    WdfRequestCancelSentRequest(Request);
}



void HfpEvtQueueIoDeviceControl (WDFQUEUE Queue, WDFREQUEST Request, size_t OutBufferLen, size_t InBufferLen, ULONG IoControlCode)
{
    HFPDEVICE_CONTEXT*	devCtx;
    NTSTATUS			status;
    PVOID				inbuf;
    size_t				size;

	UNREFERENCED_PARAMETER(InBufferLen);
	UNREFERENCED_PARAMETER(OutBufferLen);

    switch (IoControlCode) 
	{
        case IOCTL_HFP_OPEN_SCO:
			TraceEvents(TRACE_LEVEL_INFORMATION, DBG_WRITE, "HfpEvtQueueIoDeviceControl: Open\n");

			// For buffered IOCTLs WdfRequestRetrieveInputBuffer & WdfRequestRetrieveOutputBuffer return the same buffer:
			// pointer (Irp->AssociatedIrp.SystemBuffer)
			status = WdfRequestRetrieveInputBuffer(Request, 0, &inbuf, &size);
			if (!NT_SUCCESS(status)) {
				status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}

			// Also may be checked here: NT_ASSERT(size == InBufferLen)
			// We will check that the supplied buffer is our 2-words MAC Address 
			if (size != 8) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}

			devCtx = GetClientDeviceContext(WdfIoQueueGetDevice(Queue));
		    devCtx->ServerBthAddress = *((BTH_ADDR*)inbuf);
			TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, "ServerBthAddress = %llX\n", devCtx->ServerBthAddress);

			status = HfpOpenRemoteConnection(devCtx, WdfRequestGetFileObject(Request), Request);
			// if it succeeds RemoteConnectCompletion will complete the request; so return here
			if (NT_SUCCESS(status))
				return;

            break;

        case IOCTL_HFP_CLOSE_SCO:
			TraceEvents(TRACE_LEVEL_INFORMATION, DBG_WRITE, "HfpEvtQueueIoDeviceControl: Close\n");
            status = STATUS_SUCCESS;
            break;

		default:
            status = STATUS_INVALID_PARAMETER;
    }

    WdfRequestComplete (Request, status);
    return;
}

