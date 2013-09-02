/*++

Module Name:
    Device.c

Abstract:
    Device and file object related functionality for bth HFP device

Environment:
    Kernel mode only
--*/

#include "driver.h"
#include "device.h"
#include "connection.h"
#include "clisrv.h"
#include "client.h"
#include "server.h"
#include "queue.h"

#define INITGUID
#include "hfppublic.h"

#if defined(EVENT_TRACING)
#include "device.tmh"
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, HfpEvtDriverDeviceAdd)
#pragma alloc_text (PAGE, HfpEvtDeviceSelfManagedIoCleanup)
#pragma alloc_text (PAGE, HfpEvtFileCreate)
#pragma alloc_text (PAGE, HfpEvtFileCleanup)
#pragma alloc_text (PAGE, HfpEvtFileClose)
#endif


NTSTATUS HfpEvtDriverDeviceAdd (_In_ WDFDRIVER  Driver, _Inout_ PWDFDEVICE_INIT  DeviceInit)
{
    NTSTATUS                        status;
    WDFDEVICE                       device;    
    WDF_OBJECT_ATTRIBUTES           deviceAttributes;
    WDF_PNPPOWER_EVENT_CALLBACKS    pnpPowerCallbacks;
    WDF_FILEOBJECT_CONFIG           fileobjectConfig;
    WDF_OBJECT_ATTRIBUTES           fileAttributes, requestAttributes;
    WDF_IO_QUEUE_CONFIG             ioQueueConfig;
    WDFQUEUE                        queue;
    HFPDEVICE_CONTEXT			   *devCtx;
    
    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, "HfpEvtDriverDeviceAdd\n");

    // Configure PnP/Power callbacks
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDeviceSelfManagedIoInit = HfpEvtDeviceSelfManagedIoInit;
    pnpPowerCallbacks.EvtDeviceSelfManagedIoCleanup = HfpEvtDeviceSelfManagedIoCleanup;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    // Configure file callbacks
    WDF_FILEOBJECT_CONFIG_INIT (&fileobjectConfig, HfpEvtFileCreate, HfpEvtFileClose, HfpEvtFileCleanup /*WDF_NO_EVENT_CALLBACK Cleanup*/);

    // Inform framework to create context area in every fileobject so that we can track information per open handle by the application.
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&fileAttributes, HFP_FILE_CONTEXT);
    WdfDeviceInitSetFileObjectConfig (DeviceInit, &fileobjectConfig, &fileAttributes);

    // Inform framework to create context area in every request object.
    // We make BRB as the context since we need BRB for all the requests we handle (Create, Read, Write).
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE (&requestAttributes, BRB);
    WdfDeviceInitSetRequestAttributes(DeviceInit, &requestAttributes);

    // Set device attributes
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, HFPDEVICE_CONTEXT);
    status = WdfDeviceCreate (&DeviceInit, &deviceAttributes, &device);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "WdfDeviceCreate failed, Status %X", status);
        goto exit;
    }

    devCtx = GetClientDeviceContext(device);

    // Initialize our context
    status = HfpSharedDeviceContextInit(devCtx, device);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "Initialization of context failed, Status %X", status);
        goto exit;       
    }

    status = HfpBthQueryInterfaces(devCtx);        
    if (!NT_SUCCESS(status)) {
        goto exit;
    }

    // Set up our queue
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE (&ioQueueConfig, WdfIoQueueDispatchParallel);

    ioQueueConfig.EvtIoRead  = HfpEvtQueueIoRead;
    ioQueueConfig.EvtIoWrite = HfpEvtQueueIoWrite;
    ioQueueConfig.EvtIoStop  = HfpEvtQueueIoStop;
    ioQueueConfig.EvtIoDeviceControl = HfpEvtQueueIoDeviceControl;

    status = WdfIoQueueCreate (device, &ioQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, &queue);
    
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "WdfIoQueueCreate failed, Status=%X", status);
        goto exit;
    }

    // Enable device interface so that app can open a handle to our device and talk to it
    status = WdfDeviceCreateDeviceInterface(device, &HFP_DEVICE_INTERFACE, NULL);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "Enabling device interface failed, Status %X", status);
        goto exit;       
    }

	// Get Bluetooth Radio System info
	{
	struct _BRB_SCO_GET_SYSTEM_INFO* brb;

	devCtx->ProfileDrvInterface.BthReuseBrb(&(devCtx->RegisterUnregisterBrb), BRB_SCO_GET_SYSTEM_INFO);
	brb = (struct _BRB_SCO_GET_SYSTEM_INFO*) &(devCtx->RegisterUnregisterBrb);
	status = HfpSharedSendBrbSynchronously (devCtx->IoTarget, devCtx->Request, (PBRB)brb, sizeof(*brb));
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "BRB_SCO_GET_SYSTEM_INFO failed, Status %X", status);
		goto exit;        
	}
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, "BRB_SCO_GET_SYSTEM_INFO:");
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, " Features = %X", brb->Features);
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, " MaxChannels = %X", brb->MaxChannels);
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, " TransferUnit = %X", brb->TransferUnit);
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, " PacketTypes = %X", brb->PacketTypes);
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, " DataFormats = %X", brb->DataFormats);

	devCtx->ScoPacketTypes = brb->PacketTypes;
	}

	exit:
    // We don't need to worry about deleting any objects on failure because all the object created so far are parented to device and when
    // we return an error, framework will delete the device and as a result all the child objects will get deleted along with that.
    return status;
}



NTSTATUS HfpEvtDeviceSelfManagedIoInit (_In_ WDFDEVICE Device)
{
	NTSTATUS status;
	HFPDEVICE_CONTEXT* devCtx = GetClientDeviceContext(Device);

	status = HfpSharedRetrieveLocalInfo(devCtx);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "HfpSharedRetrieveLocalInfo failed");
		goto exit;
	}

	TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "HfpEvtDeviceSelfManagedIoInit: LocalBthAddr %X", devCtx->LocalBthAddr);

	/*status = */HfpSrvPublishSdpRecord(devCtx->Device);

	exit:
	return status;
}


void HfpEvtDeviceSelfManagedIoCleanup (_In_ WDFDEVICE  Device)
{
	HFPDEVICE_CONTEXT* devCtx = GetClientDeviceContext(Device);

	PAGED_CODE();
	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, "HfpEvtDeviceSelfManagedIoCleanup, ScoServerHandle = %X", devCtx->ScoServerHandle);

	if (devCtx->SdpRecordHandle)
		HfpSrvRemoveSdpRecord(devCtx);

	if (devCtx->ScoServerHandle)
		HfpSrvUnregisterScoServer(devCtx, 0);

	// After this point no more connections can come because we have unregistered server.
}


void HfpEvtFileCreate (_In_ WDFDEVICE Device, _In_ WDFREQUEST Request, _In_ WDFFILEOBJECT fileObject)
{
	NTSTATUS status = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(Device);

	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_PNP, "HfpEvtFileCreate");

	if (GetClientDeviceContext(Device)->FileObject)	{
		TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "ERROR: Device is already open");
		status = STATUS_INVALID_DEVICE_REQUEST;
		goto exit;
	}
	else  {
		GetClientDeviceContext(Device)->FileObject = fileObject;
	}

	// Must be cleared before we may open it in the HfpEvtQueueIoDeviceControl,IOCTL_HFP_OPEN_SCO
	// In order to check this state when closing
	NT_ASSERT (GetFileContext(fileObject)->Connection == 0);

	PAGED_CODE();

	// All the activity of this function is moved to HfpEvtQueueIoDeviceControl-IOCTL_HFP_OPEN_SCO,
	// because of the user application first must supply the destination Bluetooth device address,
	// and only then the SCO channel may be opened and the tx/rx will start.
	// As result this function is empty.

	exit:
	WdfRequestComplete(Request, status);
}


void HfpEvtFileCleanup (_In_ WDFFILEOBJECT  fileObject)
{
	HFPDEVICE_CONTEXT*	devCtx = GetClientDeviceContext(WdfFileObjectGetDevice(fileObject));
	HFP_CONNECTION*		connection = GetFileContext(fileObject)->Connection;
	PAGED_CODE();
	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_PNP, "HfpEvtFileCleanup, fileObject=%X", fileObject);
	NT_ASSERT (devCtx->FileObject == fileObject);
	devCtx->FileObject = 0;

	// Since this routine is called at passive level we can disconnect synchronously
	if (connection)
		HfpConnectionObjectRemoteDisconnectSynchronously (devCtx, connection);
}


void HfpEvtFileClose (_In_ WDFFILEOBJECT  fileObject)
{
	//HFPDEVICE_CONTEXT*	devCtx     = GetClientDeviceContext(WdfFileObjectGetDevice(fileObject));
	HFP_CONNECTION*		connection = GetFileContext(fileObject)->Connection;
	PAGED_CODE();
	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_PNP, "HfpEvtFileClose, fileObject=%X, connection=%X", fileObject, connection);
}

