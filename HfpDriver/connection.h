/*++

Module Name:
    Connection.h

Abstract:
    Connection object which represents an SCO connection

    We create a WDFOBJECT for connection in HfpConnectionObjectCreate. 
    PHFP_CONNECTION is maintained as context of this object.

    We use passive dispose for connection WDFOBJECT. This allows us to wait for
    continuous readers to stop and disconnect to complete in the cleanup
    callback (HfpEvtConnectionObjectCleanup) of connection object.

    Connection object also includes continuous readers which need 
    to be explicitly initialized and submitted.

    Both client and server utilize connection object although continuous reader
    functionality is utilized only by server.
	
Environment:
    Kernel mode
--*/

typedef struct _HFP_CONNECTION* PHFP_CONNECTION;


#define HFP_NUM_CONTINUOUS_READERS	2


typedef void
(*PFN_HFP_CONNECTION_OBJECT_CONTREADER_READ_COMPLETE) (
    _In_ HFPDEVICE_CONTEXT_HEADER* DevCtxHdr,
    _In_ PHFP_CONNECTION Connection,
    _In_ PVOID Buffer,
    _In_ size_t BufferSize
    );

typedef void
(*PFN_HFP_CONNECTION_OBJECT_CONTREADER_FAILED) (
    _In_ HFPDEVICE_CONTEXT_HEADER* DevCtxHdr,
    _In_ PHFP_CONNECTION Connection
    );


typedef struct
{
    struct _BRB_SCO_TRANSFER	TransferBrb;		// BRB used for transfer
    WDFREQUEST					RequestPendingRead;	// WDF Request used for pending I/O
    WDFMEMORY					MemoryPendingRead;	// Data buffer for pending read
    KDPC						ResubmitDpc;		// Dpc for resubmitting pending read
    LONG						Stopping;			// Whether the continuous reader is transitioning to stopped state
    KEVENT						StopEvent;			// Event used to wait for read completion while stopping the continuous reader
    PHFP_CONNECTION				Connection;			// Back pointer to connection
} HFP_REPEAT_READER;



typedef enum {
    ConnectionStateUnitialized = 0,
    ConnectionStateInitialized,
    ConnectionStateConnecting,
    ConnectionStateConnected,
    ConnectionStateConnectFailed,
    ConnectionStateDisconnecting,
    ConnectionStateDisconnected
} HFP_CONNECTION_STATE;


typedef struct 
{
    HFP_REPEAT_READER										RepeatReaders[HFP_NUM_CONTINUOUS_READERS];
    PFN_HFP_CONNECTION_OBJECT_CONTREADER_READ_COMPLETE		ReaderCompleteCallback;
    PFN_HFP_CONNECTION_OBJECT_CONTREADER_FAILED             ReaderFailedCallback;
    DWORD													InitializedReadersCount;
} HFP_CONTINUOUS_READER;



//
// Connection data structure for SCO connection
//

typedef struct _HFP_CONNECTION
{
    LIST_ENTRY                      ConnectionListEntry;	// List entry for connection list maintained at device level
    HFPDEVICE_CONTEXT_HEADER*		DevCtxHdr;    
    HFP_CONNECTION_STATE			ConnectionState;
    WDFSPINLOCK                     ConnectionLock;			// Connection lock, used to synchronize access to _HFP_CONNECTION data structure
    USHORT                          OutMTU;
    USHORT                          InMTU;
    SCO_CHANNEL_HANDLE				ChannelHandle;
    BTH_ADDR                        RemoteAddress;
    struct _BRB                     ConnectDisconnectBrb;	// Preallocated Brb, Request used for connect/disconnect
    WDFREQUEST                      ConnectDisconnectRequest;
    KEVENT                          DisconnectEvent;		// Event used to wait for disconnection - it is non-signaled when connection is in ConnectionStateDisconnecting; transitionary state and signaled otherwise
    HFP_CONTINUOUS_READER			ContinuousReader;		// Continuous readers (used only by server). PLEASE NOTE that KMDF USB Pipe Target uses a single continuous reader
} HFP_CONNECTION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(HFP_CONNECTION, GetConnectionObjectContext)



/*
  This routine creates a connection object.
  This is called by client and server when a remote connection is made.

 Arguments:
    DevCtxHdr - Device context header
    ParentObject - Parent object for connection object
    ConnectionObject - receives the created connection object

 Return Value:
    NTSTATUS Status code.
--*/
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS HfpConnectionObjectCreate(_In_ HFPDEVICE_CONTEXT_HEADER* DevCtxHdr, _In_ WDFOBJECT ParentObject, _Out_ WDFOBJECT*  ConnectionObject);



/*
  This routine initializes continuous reader for connection

  Arguments:
    Connection	- Connection for which to initialize continuous readers
    HfpConnectionObjectContReaderReadCompleteCallback - Pending read completion callback
    HfpConnectionObjectContReaderFailedCallback - Repeat reader failed callback
    BufferSize - Buffer size for the pending read

Return Value:
    NTSTATUS Status code.
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS HfpConnectionObjectInitializeContinuousReader(
    _In_ HFP_CONNECTION* Connection,
    _In_ PFN_HFP_CONNECTION_OBJECT_CONTREADER_READ_COMPLETE HfpConnectionObjectContReaderReadCompleteCallback,
    _In_ PFN_HFP_CONNECTION_OBJECT_CONTREADER_FAILED        HfpConnectionObjectContReaderFailedCallback,
    _In_ size_t BufferSize
    );



/*
 Description:
    This routine submits all the initialized repeat readers for the connection

 Arguments:
    Connection - Connection whose repeat readers are to be submitted

 Return Value:
    NTSTATUS Status code.
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS HfpConnectionObjectContinuousReaderSubmitReaders (_In_ HFP_CONNECTION* Connection);


/*
  This routine cancels all the initialized repeat readers for the connection
  Arguments:
    Connection - Connection whose repeat readers are to be canceled
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
void HfpConnectionObjectContinuousReaderCancelReaders(_In_ HFP_CONNECTION* Connection);



/*
 This routine sends a disconnect BRB for the connection

 Arguments:
    DevCtxHdr  - Device context header
    Connection - Connection which is to be disconnected

 Return Value:
    TRUE is this call initiates the disconnect.
    FALSE if the connection was already disconnected.
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN HfpConnectionObjectRemoteDisconnect(_In_ HFPDEVICE_CONTEXT_HEADER* DevCtxHdr, _In_ HFP_CONNECTION* Connection);



/*
 This routine disconnects the connection synchronously

 Arguments:
    DevCtxHdr  - Device context header
    Connection - Connection which is to be disconnected
*/
_IRQL_requires_max_(PASSIVE_LEVEL)
void HfpConnectionObjectRemoteDisconnectSynchronously (_In_ HFPDEVICE_CONTEXT_HEADER* DevCtxHdr, _In_ HFP_CONNECTION* Connection);



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
    DevCtxHdr		 - Device context header

 Return Value:
    NTSTATUS Status code.
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS HfpConnectionObjectInit (_In_ WDFOBJECT ConnectionObject, _In_ HFPDEVICE_CONTEXT_HEADER* DevCtxHdr);



/*
 This routine submits the repeat reader. In case of failure it invoked contreader failed callback

 Arguments:
    DevCtxHdr - Device context header
    RepeatReader - Repeat reader to submit

 Return Value:
    NTSTATUS Status code.
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS HfpRepeatReaderSubmit(_In_ HFPDEVICE_CONTEXT_HEADER* DevCtx, _In_ HFP_REPEAT_READER* RepeatReader);


/*
 This routine waits for repeat reader to stop
 Arguments:
    RepeatReader - Repeat reader to be waited on
*/
_IRQL_requires_max_(PASSIVE_LEVEL)
void HfpRepeatReaderWaitForStop(_In_ HFP_REPEAT_READER* RepeatReader);



/*
 This routine waits for and uninitializes all the initialized continuous readers for the connection.
 Arguments:
    Connection - Connection whose continuous reader is to be uninitialized
*/
_IRQL_requires_max_(PASSIVE_LEVEL)
void HfpConnectionObjectWaitForAndUninitializeContinuousReader(_In_ HFP_CONNECTION* Connection);


/*
 Dpc for resubmitting pending read

 Arguments:
    Dpc - Dpc
    DeferredContext - We receive device context header as DeferredContext
    SystemArgument1 - We receive repeat reader as SystemArgument1
    SystemArgument2 - Unused
*/
KDEFERRED_ROUTINE HfpRepeatReaderResubmitReadDpc;



/*
  Completion routine for pending read request
  In this routine we invoke the read completion callback in case of success and contreader failure callback 
  in case of failure. These are implemented by server.

Arguments:
    Request - Request completed
    Target  - Target to which request was sent
    Params  - Completion parameters
    Context - We receive repeat reader as the context
*/
EVT_WDF_REQUEST_COMPLETION_ROUTINE HfpRepeatReaderPendingReadCompletion;


/*
 This routine initializes repeat reader.

 Arguments:
    Connection - Connection with which this repeat reader is associated
    RepeatReader - Repeat reader
    BufferSize - Buffer size for read

 Return Value:
    NTSTATUS Status code.
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS HfpRepeatReaderInitialize(_In_ HFP_CONNECTION* Connection, _In_ HFP_REPEAT_READER* RepeatReader, _In_ size_t BufferSize);


/*
  This routine cancels repeat reader but it does not wait for repeat reader to stop.
  This wait must be performed separately.

 Arguments:
    RepeatReader - Repeat reader to be canceled
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
void HfpRepeatReaderCancel(_In_ HFP_REPEAT_READER* RepeatReader);
