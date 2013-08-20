/*++

Module Name:
    Device.h

Abstract:
    Context and function declarations for bth HFP device

Environment:
    Kernel mode only
--*/


#include "clisrv.h"


typedef struct
{
    HFPDEVICE_CONTEXT_HEADER	Header;					// Context common to client and server
    BTH_ADDR                    ServerBthAddress;		// Destination Bluetooth address
    HANDLE_SDP					SdpRecordHandle;		// Handle to published SDP record
    SCO_SERVER_HANDLE			ScoServerHandle;		// Handle obtained by registering SCO server
    struct _BRB					RegisterUnregisterBrb;	// BRB used for server register
    WDFSPINLOCK					ConnectionListLock;		// Connection List lock
    LIST_ENTRY					ConnectionList;			// Outstanding open connections
} HFPDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(HFPDEVICE_CONTEXT, GetClientDeviceContext)


typedef struct
{
    HFP_CONNECTION*	 Connection;						// Connection to server opened for this file
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
    FileObject	- File object corresponding to Create
*/
EVT_WDF_DEVICE_FILE_CREATE		HfpEvtDeviceFileCreate;



/*
 This routine is invoked by Framework when I/O manager sends Close IRP for a file.
 In response we close the remote connection to the server.

 Arguments:
    FileObject - File object corresponding to Close
*/
EVT_WDF_FILE_CLOSE	 HfpEvtFileClose;



/*
 This routine is invoked by HfpSrvRemoteConnectResponseCompletion when connect 
 response is completed. We initialize and submit continuous readers in this routine.

 Arguments:
    ConnectionObject - connection object for which connect response completed
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS HfpSrvConnectionStateConnected (WDFOBJECT ConnectionObject);



