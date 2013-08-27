/*++

Module Name:
    client.h

Abstract:
    Contains declarations for Bluetooth client functionality.
    
Environment:
    Kernel mode only
--*/



/*
 Query profile driver interface and store it in device context

 Arguments:
    devCtx - Client context where we store the interface

 Return Value:
    NTSTATUS Status code.
*/
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS HfpBthQueryInterfaces(_In_ HFPDEVICE_CONTEXT* devCtx);


/*
 This routine is invoked by HfpEvtDeviceFileCreate. In this routine we send down open channel BRB.
 This routine allocates open channel BRB. If the request is sent down successfully completion routine needs to free this BRB.
    
 Arguments:
    _In_ BTHECHOSAMPLE_CLIENT_CONTEXT* devCtx - 
    _In_ WDFFILEOBJECT fileObject - 
    _In_ WDFREQUEST Request - 

 Return Value:
    NTSTATUS Status code.
*/
_IRQL_requires_same_
NTSTATUS HfpOpenRemoteConnection(_In_ HFPDEVICE_CONTEXT* devCtx, _In_ WDFFILEOBJECT fileObject, _In_ WDFREQUEST Request);


/*
 Indication callback passed to bth stack while sending open channel BRB
 Bth stack sends notification related to the connection.
    
 Arguments:

    Context - We receive data connection as the context
    Indication - Type of indication
    Parameters - Parameters of indication
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
#if (NTDDI_VERSION >= NTDDI_WIN8)
void HfpIndicationCallback (_In_ PVOID Context, _In_ INDICATION_CODE Indication, _In_ PINDICATION_PARAMETERS_ENHANCED Parameters);
#else
void HfpIndicationCallback (_In_ PVOID Context, _In_ SCO_INDICATION_CODE Indication, _In_ PSCO_INDICATION_PARAMETERS Parameters);
#endif



EVT_WDF_REQUEST_COMPLETION_ROUTINE	HfpRemoteConnectCompletion;


/*++
 Completion routine for Create request which we format as open channel BRB and 
 send down the stack. We complete the Create request in this routine.
 
 We receive open channel BRB as the context. This BRB is part of the request context 
 and doesn't need to be freed explicitly.
 
 Connection is part of the context in the BRB.

 Arguments:
    Request - Create request that we formatted with open channel BRB
    Target - Target to which we sent the request
    Params - Completion params
    Context - We receive BRB as the context          

 Return Value:
    NTSTATUS Status code.
*/
void HfpRemoteConnectCompletion (_In_ WDFREQUEST  Request, _In_ WDFIOTARGET  Target, _In_ PWDF_REQUEST_COMPLETION_PARAMS  Params, _In_ WDFCONTEXT  Context);

