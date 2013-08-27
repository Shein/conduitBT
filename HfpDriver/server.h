/*++

Module Name:
    Server.h

Abstract:
    Contains declarations for Bluetooth server functionality.
    
Environment:
    Kernel mode only
--*/


/*
 Indication callback passed to bth stack while registering server and for responding to connect. 
 Bth stack sends notification related to the server and connection. 
 We receive connect and disconnect notifications in this callback.

 Arguments:
    Context		- server device context
    Indication	- type of indication
    Parameters	- parameters of indication
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
void HfpSrvIndicationCallback(_In_ void* Context, _In_ SCO_INDICATION_CODE Indication, _In_ SCO_INDICATION_PARAMETERS* Parameters);



/*
 Publishes server SDP record. We first build SDP record using CreateSdpRecord in sdp.c 
 and call this function to publish the record.

 Arguments:
    devCtx			- Device context of the server
    SdpRecord		- SDP record to publish
    SdpRecordLength - SDP record length
*/
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS HfpSrvPublishSdpRecord (_In_ WDFDEVICE Device);



/*
 Removes SDP record

 Arguments:
    devCtx - Device context of the server
*/
_IRQL_requires_max_(PASSIVE_LEVEL)
void HfpSrvRemoveSdpRecord (_In_ HFPDEVICE_CONTEXT* devCtx);


/*
 Registers SCO server.

 Arguments:
    devCtx	  - Device context
	regparams - Server parameters (Address of the remote Bluetooth device to receive notifications for, User mode handles)

Return Value:
    NTSTATUS Status code.
*/
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS HfpSrvRegisterScoServer (_In_ HFPDEVICE_CONTEXT* devCtx, _In_ HFP_REG_SERVER* regparams);


/*
 Unregisters SCO server.

 Arguments:
    devCtx	 - Device context
	destaddr - optional destination address that was registered before. If not 0, the caller wants 
	           actually do unregister+register new device. The referenced address will be compared 
			   with previously one registered: if it is the same - no unregistering action will be 
			   performed. The caller may then test devCtx->ScoServerHandle, if it's not cleared
			   it means no unregister/register needed.
 Return Value:
    NTSTATUS Status code.
*/
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS HfpSrvUnregisterScoServer (_In_ HFPDEVICE_CONTEXT* devCtx, _Inout_ UINT64 destaddr);


/*
 Respond to connect indication received. This function is invoked by 
 HfpSrvIndicationCallback when connect indication is received.

 Arguments:
    devCtx		  - Server device context
    ConnectParams - Connect indication parameters
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS HfpSrvSendConnectResponse (_In_ HFPDEVICE_CONTEXT* devCtx, _In_ SCO_INDICATION_PARAMETERS* ConnectParams);


/*
 Disconnects connection, removes it from the list and deletes the connection object.   
 This helper function can be called for disconnect only after connect is completed.

 Arguments:
    Connection - Connection to disconnect
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID HfpSrvDisconnectConnection (_In_ HFP_CONNECTION * Connection);


/*
 Completion routine for BRB_L2CA_OPEN_CHANNEL_RESPONSE BRB sent in BthEchoSrvSendConnectResponse.
 If the request completed successfully we call BthEchoSrvConnectionStateConnected function implemented 
 in device.c. This function initializes and submits continuous readers to read the data coming from 
 client which then is used for echo.

 Arguments:
    Request - Request that completed
    Target	- Target to which request was sent
    Params	- Request completion parameters
    Context - We receive BRB as the context. This BRB is connection->ConnectDisconnectBrb, 
	          and is not allocated separately hence it is not freed here.
*/
EVT_WDF_REQUEST_COMPLETION_ROUTINE  HfpSrvRemoteConnectCompletion;
