/*++

 Module Name:
    Connection.h

 Abstract:
    Connection object which represents an SCO connection

    We create a WDFOBJECT for connection in HfpConnectionObjectCreate. 
    HFP_CONNECTION* is maintained as context of this object.

    We use passive dispose for connection WDFOBJECT. This allows us to wait for
    continuous readers to stop and disconnect to complete in the cleanup
    callback (HfpEvtConnectionObjectCleanup) of connection object.

    Connection object also includes continuous readers which need 
    to be explicitly initialized and submitted.

    Both client and server utilize connection object although continuous reader
    functionality is utilized only by server.

	HFP driver doesn't use the continuous reader at all: all IO operations are 
	initiated by User Mode applications.
	
 Environment:
    Kernel mode
--*/



typedef enum {
    ConnectionStateUnitialized = 0,
    ConnectionStateInitialized,
    ConnectionStateConnecting,
    ConnectionStateConnected,
    ConnectionStateConnectFailed,
    ConnectionStateDisconnecting,
    ConnectionStateDisconnected
} HFP_CONNECTION_STATE;


/*
  SCO Connection context
*/
typedef struct HFP_CONNECTION
{
    HFPDEVICE_CONTEXT*		DevCtx;						// This device context
	WDFFILEOBJECT			FileObject;					// This connection parent FileObject
    HFP_CONNECTION_STATE	ConnectionState;			// Connection state for connect/disconnect handshake
    WDFSPINLOCK             ConnectionLock;				// Connection lock, used to synchronize access to HFP_CONNECTION members
    SCO_CHANNEL_HANDLE		ChannelHandle;				// SCO channel handle
    BTH_ADDR				RemoteAddress;				// Remote device address
    struct _BRB				ConnectDisconnectBrb;		// Preallocated BRB request used for connect/disconnect
    WDFREQUEST				ConnectDisconnectRequest;	// WDF Request for connect/disconnect
    KEVENT					DisconnectEvent;			// Event used to wait for disconnection - it is non-signaled when connection is in ConnectionStateDisconnecting; transitionary state and signaled otherwise
} HFP_CONNECTION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(HFP_CONNECTION, GetConnectionObjectContext)



/*
  This routine creates a connection object.
  This is called by client and server when a remote connection is made.

 Arguments:
    DevCtx - Device context header
    parentObject - Parent object for connection object
    ConnectionObject - receives the created connection object

 Return Value:
    NTSTATUS Status code.
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS HfpConnectionObjectCreate(_In_ HFPDEVICE_CONTEXT* devCtx, _In_ WDFOBJECT parentObject, _Out_ WDFOBJECT*  ConnectionObject);



/*
 This routine sends a disconnect BRB for the connection

 Arguments:
    devCtx  - Device context header
    Connection - Connection which is to be disconnected

 Return Value:
    TRUE is this call initiates the disconnect.
    FALSE if the connection was already disconnected.
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN HfpConnectionObjectRemoteDisconnect(_In_ HFPDEVICE_CONTEXT* devCtx, _In_ HFP_CONNECTION* Connection);



/*
 This routine disconnects the connection synchronously

 Arguments:
    devCtx  - Device context header
    Connection - Connection which is to be disconnected
*/
_IRQL_requires_max_(PASSIVE_LEVEL)
void HfpConnectionObjectRemoteDisconnectSynchronously (_In_ HFPDEVICE_CONTEXT* devCtx, _In_ HFP_CONNECTION* Connection);



/*
 Formats a request for SCO transfer

 Arguments:
    Connection	- Connection on which SCO transfer will be made
    Request		- Request to be formatted
    Brb			- If a Brb is passed in, it will be used, otherwise this routine will allocate the Brb and return in this parameter
    Memory		- Memory object which has the buffer for transfer
    TransferFlags - Transfer flags which include direction of the transfer

 Return Value:
    NTSTATUS Status code.
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
_When_(return==0, _At_(*Brb, __drv_allocatesMem(Mem)))
NTSTATUS
HfpConnectionObjectFormatRequestForScoTransfer(
    _In_ HFP_CONNECTION* Connection,
    _In_ WDFREQUEST Request,
    _Inout_ struct _BRB_SCO_TRANSFER ** Brb,
    _In_ WDFMEMORY Memory,
    _In_ ULONG TransferFlags //flags include direction of transfer
    );



/*
 This routine is invoked by the Framework when connection object  gets deleted 
 (either explicitly or implicitly because of parent deletion).

 Since we mark ExecutionLevel as passive for connection object we get this callback at passive level 
 and can wait for stopping of continuous readers and for disconnect to complete.

Arguments:
    ConnectionObject - The Connection Object

Return Value:
    None
*/
EVT_WDF_OBJECT_CONTEXT_CLEANUP HfpEvtConnectionObjectCleanup;



/*
 This routine formats are WDFREQUEST with the passed in BRB

 Arguments:
    IoTarget - Target to which request will be sent
    Request - Request to be formatted
    Brb - BRB to format the request with
    BrbSize - size of the BRB
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS FormatRequestWithBrb(
    _In_ WDFIOTARGET IoTarget,
    _In_ WDFREQUEST Request,
    _In_ PBRB Brb,
    _In_ size_t BrbSize
    );


EVT_WDF_REQUEST_COMPLETION_ROUTINE HfpConnectionObjectDisconnectCompletion;


/*
 Completion routine for disconnect BRB completion.
 In this routine we set the connection to disconnect and set the DisconnectEvent.

Arguments:
    Request - Request completed
    Target	- Target to which request was sent
    Params	- Completion parameters
    Context - We receive connection as the context
*/
void
HfpConnectionObjectDisconnectCompletion(
    _In_ WDFREQUEST  Request,
    _In_ WDFIOTARGET  Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS  Params,
    _In_ WDFCONTEXT  Context
    );



/*
  This routine initializes connection object.  It is invoked by HfpConnectionObjectCreate.

 Arguments:
    ConnectionObject - Object to initialize
    devCtx		 - Device context header

 Return Value:
    NTSTATUS Status code.
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS HfpConnectionObjectInit (_In_ WDFOBJECT ConnectionObject, _In_ HFPDEVICE_CONTEXT* devCtx, _In_ WDFOBJECT parentObject);

