/*++

Module Name:
    Device.c

Abstract:
    Device and file object related functionality for bth HFP device

Environment:
    Kernel mode only
--*/

#include "device.h"
#include "clisrv.h"
#include "client.h"
#include "queue.h"

#define INITGUID
#include "hfppublic.h"

#if defined(EVENT_TRACING)
#include "device.tmh"
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, HfpEvtDriverDeviceAdd)
#pragma alloc_text (PAGE, HfpEvtDeviceFileCreate)
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

    // Configure Pnp/power callbacks
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDeviceSelfManagedIoInit = HfpEvtDeviceSelfManagedIoInit;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    // Configure file callbacks
    WDF_FILEOBJECT_CONFIG_INIT (&fileobjectConfig, HfpEvtDeviceFileCreate, HfpEvtFileClose, WDF_NO_EVENT_CALLBACK /*Cleanup*/);

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
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "WdfDeviceCreate failed with Status code %!STATUS!\n", status);
        goto exit;
    }

    devCtx = GetClientDeviceContext(device);

    // Initialize our context
    status = HfpSharedDeviceContextHeaderInit(&devCtx->Header, device);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "Initialization of context failed with Status code %!STATUS!\n", status);
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
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "WdfIoQueueCreate failed  %!STATUS!\n", status);
        goto exit;
    }

    // Enable device interface so that app can open a handle to our device and talk to it
    status = WdfDeviceCreateDeviceInterface(device, &HFP_DEVICE_INTERFACE, NULL);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP, "Enabling device interface failed with Status code %!STATUS!\n", status);
        goto exit;       
    }

	exit:    
    // We don't need to worry about deleting any objects on failure because all the object created so far are parented to device and when
    // we return an error, framework will delete the device and as a result all the child objects will get deleted along with that.
    return status;
}



NTSTATUS HfpEvtDeviceSelfManagedIoInit (_In_ WDFDEVICE  Device)
{
    HFPDEVICE_CONTEXT* devCtx = GetClientDeviceContext(Device);
    return HfpSharedRetrieveLocalInfo(&devCtx->Header);
}



void HfpEvtDeviceFileCreate (_In_ WDFDEVICE Device, _In_ WDFREQUEST Request, _In_ WDFFILEOBJECT FileObject)
{
    NTSTATUS status = STATUS_SUCCESS;

    // Must be cleared before we may open it in the HfpEvtQueueIoDeviceControl,IOCTL_HFP_OPEN_SCO
	// In order to check this state when closing
	GetFileContext(FileObject)->Connection = 0;

	UNREFERENCED_PARAMETER(Device);
        
    PAGED_CODE();
    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_PNP, "HfpEvtDeviceFileCreate\n");

	// All the activity of this function is moved to HfpEvtQueueIoDeviceControl-IOCTL_HFP_OPEN_SCO,
	// because of the user application first must supply the destination Bluetooth device address,
	// and only then the SCO channel may be opened and the tx/rx will start.
	// As result this function is empty.

    WdfRequestComplete(Request, status);        
}



void HfpEvtFileClose (_In_ WDFFILEOBJECT  FileObject)
{    
    HFPDEVICE_CONTEXT*	devCtx;
    HFP_CONNECTION* connection;

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_PNP, "HfpEvtFileClose\n");

    devCtx = GetClientDeviceContext(WdfFileObjectGetDevice(FileObject));
    connection = GetFileContext(FileObject)->Connection;

    // Since this routine is called at passive level we can disconnect synchronously.
	if (connection)
		HfpConnectionObjectRemoteDisconnectSynchronously (&(devCtx->Header), connection);
}
