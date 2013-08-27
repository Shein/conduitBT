/*++

Module Name:
    Driver.c

Abstract:
    Driver object related declarations for bth HFP device: WDFDRIVER Events

Environment:
    Kernel mode only
--*/


#include <ntddk.h>
#include <wdm.h>
#include <wdf.h>
#include <initguid.h> 
#include <ntstrsafe.h>
#include <bthdef.h>
#include <ntintsafe.h>
#include <bthguid.h>
#include <bthioctl.h>
#include <sdpnode.h>
#include <bthddi.h>
#include <bthsdpddi.h>
#include <bthsdpdef.h>



/*
 DriverEntry initializes the driver and is the first routine called by the system after the driver is loaded.

 Parameters Description:
    DriverObject - represents the instance of the function driver that is loaded into memory. DriverEntry must initialize members of DriverObject before it
    returns to the caller. DriverObject is allocated by the system before the driver is loaded, and it is released by the system after the system unloads
    the function driver from memory.

    RegistryPath - represents the driver specific path in the Registry. The function driver can use the path to store driver related data between
    reboots. The path does not store hardware instance specific data.

 Return Value:
    STATUS_SUCCESS if successful,  STATUS_UNSUCCESSFUL otherwise.
*/
DRIVER_INITIALIZE DriverEntry;



/*
 HfpEvtDeviceAdd is called by the framework in response to AddDevice call from the PnP manager. 
 We create and initialize a device object to represent a new instance of the device. All the software resources should be allocated in this callback.

 Arguments:
    Driver		- Handle to a framework driver object created in DriverEntry
    DeviceInit	- Pointer to a framework-allocated WDFDEVICE_INIT structure.

 Return Value:  NTSTATUS
*/
EVT_WDF_DRIVER_DEVICE_ADD	HfpEvtDriverDeviceAdd;




/*
 Free resources allocated in DriverEntry that are automatically cleaned up framework.

 Arguments:
    DriverObject - handle to a WDF Driver object.

 Return Value: VOID
*/
EVT_WDF_OBJECT_CONTEXT_CLEANUP	HfpEvtDriverCleanup;

