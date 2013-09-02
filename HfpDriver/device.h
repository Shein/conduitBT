/*++

 Module Name:
    Device.h

 Abstract:
    Context and function declarations for bth HFP device

 Environment:
    Kernel mode only
--*/

#include "trace.h"
#include "hfppublic.h"


/*
  HFP Device context
*/
typedef struct
{
    WDFDEVICE						Device;					// Framework device this context is associated with
    WDFIOTARGET						IoTarget;				// Default I/O target
    BTH_PROFILE_DRIVER_INTERFACE	ProfileDrvInterface;	// Profile driver interface which contains profile driver DDI
    WDFREQUEST						Request;				// Preallocated request to be reused during initialization/deinitialzation phase, access to this request is not synchronized
	WDFFILEOBJECT					FileObject;				// Current file object or 0, if no application currently opened us (we support only one application at a time)
    BTH_ADDR						LocalBthAddr;			// Local Bluetooth Address
    BTH_ADDR						RemoteBthAddress;		// Destination Bluetooth address
    HANDLE_SDP						SdpRecordHandle;		// Handle to published SDP record
    SCO_SERVER_HANDLE				ScoServerHandle;		// Handle obtained by registering SCO server
    struct _BRB						RegisterUnregisterBrb;	// BRB used for server register
	BOOLEAN							ConnectReadiness;		// To confirm incoming SCO connection only when this flag is true
    USHORT							ScoPacketTypes;			// Supported (e)SCO packet types: taken from BT Radio and then passed when opening SCOs
	PKEVENT							KevScoConnect;			// SCO Connect Event associated with correspondent User-mode Event
	PKEVENT							KevScoDisconnect;		// SCO Disconnect Event associated with correspondent User-mode Event
	#if (NTDDI_VERSION >= NTDDI_WIN8)
    BTH_HOST_FEATURE_MASK			LocalFeatures;			// Features supported by the local stack
	#endif
} HFPDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(HFPDEVICE_CONTEXT, GetClientDeviceContext)

struct HFP_CONNECTION;


/*
  HFP Device context for File (CreateFile)
*/
typedef struct
{
    struct HFP_CONNECTION *	 Connection;		// Connection opened for this file (only one connection per file supported)
} HFP_FILE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(BRB, GetRequestContext);    

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(HFP_FILE_CONTEXT, GetFileContext);



/*
  See it in driver.h
*/
EVT_WDF_DRIVER_DEVICE_ADD	HfpEvtDriverDeviceAdd;


/*
 This routine is called by the framework only once and hence we use it for our one time initialization.
 Bth addresses for both our local device and server device do not change, hence we retrieve them here.
 Features that the local device supports also do not change, so checking for local SCO support is done here and 
 saved in the device context header.
 Please note that retrieving server bth address does not require presence of the server. It is remembered from
 the installation time when the client gets installed for a specific server.

 Arguments:
    Device - Framework device object

 Return Value:
    NTSTATUS Status code.
*/
EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT		HfpEvtDeviceSelfManagedIoInit;


/*
 This routine is called by the framework only once and hence we use it for our one time de-initialization.
 In this routine we remove SDP record and unregister server. We also disconnect all the outstanding connections.

 Arguments:
    Device - Framework device object

 Return Value:
    NTSTATUS Status code.
*/
EVT_WDF_DEVICE_SELF_MANAGED_IO_CLEANUP  HfpEvtDeviceSelfManagedIoCleanup;



/*
 This routine is invoked by Framework when an application opens a handle to our device.
 In response we open a remote connection to the server.

 Arguments:
    Device		- Framework device object
    Request		- Create request
    fileObject	- File object corresponding to Create
*/
EVT_WDF_DEVICE_FILE_CREATE		HfpEvtFileCreate;



/*
 This routine is invoked by Framework when I/O manager sends Close IRP for a file.
 In response we close the remote connection to the server.

 Arguments:
    fileObject - File object corresponding to Close
*/
EVT_WDF_FILE_CLOSE	 HfpEvtFileClose;



/*
 The framework calls a driver's EvtFileCleanup callback function when the last handle to 
 the specified file object has been closed. (Because of outstanding I/O requests, this 
 handle might not have been released.) 
 After the framework calls a driver's EvtFileCleanup callback function, it calls the driver's 
 EvtFileClose callback function.
 The EvtFileCleanup callback function is called synchronously, in the context of the thread that 
 closed the last file object handle.

 Arguments:
    fileObject - File object corresponding to Close
*/
EVT_WDF_FILE_CLEANUP	HfpEvtFileCleanup;
