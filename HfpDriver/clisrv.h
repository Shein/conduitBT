/*++

Module Name:
    clisrv.h

Abstract:
    Header file for functions common to bth HFP server and client

Environment:
    Kernel mode
--*/

#pragma once

#include <ntddk.h>
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

#include "trace.h"
#include "hfppublic.h"

#define POOLTAG_BTHECHOSAMPLE 'htbw'


/*
  Device context common to both client and the server
*/
typedef struct
{
    WDFDEVICE						Device;					// Framework device this context is associated with
    WDFIOTARGET						IoTarget;				// Default I/O target
    BTH_PROFILE_DRIVER_INTERFACE	ProfileDrvInterface;	// Profile driver interface which contains profile driver DDI
    BTH_ADDR						LocalBthAddr;			// Local Bluetooth Address
#if (NTDDI_VERSION >= NTDDI_WIN8)
    BTH_HOST_FEATURE_MASK			LocalFeatures;			// Features supported by the local stack
#endif
    WDFREQUEST						Request;				// Preallocated request to be reused during initialization/deinitialzation phase, access to this request is not synchronized
} HFPDEVICE_CONTEXT_HEADER;



#include "connection.h"



/*
 Initializes the common context header between server and client

 Arguments:
    Header - Context header
    Device - Framework device object

 Return Value:
    NTSTATUS Status code.
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS HfpSharedDeviceContextHeaderInit(HFPDEVICE_CONTEXT_HEADER* header, WDFDEVICE Device);


/*
 Retrieves the local bth address. This address is burnt into device hence doesn't change.
 It also retrieves the local host supported features if available.

 Arguments:
    Header - Context header

 Return Value:
    NTSTATUS Status code.
*/
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS HfpSharedRetrieveLocalInfo(_In_ HFPDEVICE_CONTEXT_HEADER* DevCtxHdr);


#if (NTDDI_VERSION >= NTDDI_WIN8)

/*
 This routine synchronously checks the local stack's supported features

 Arguments:
    DevCtxHdr - Information about the local device

 Return Value:
    NTSTATUS Status code.
*/
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS HfpSharedGetHostSupportedFeatures(_In_ HFPDEVICE_CONTEXT_HEADER* DevCtxHdr);

#endif



/*
 This routine formats a request with BRB and sends it synchronously

 Arguments:
    IoTarget - Target to send the BRB to
    Request	 - request object to be formatted with BRB                
    Brb		 - Brb to be sent
    BrbSize	 - size of the Brb data structure

 Return Value:
    NTSTATUS Status code.

 Notes:
    This routine does calls WdfRequestReuse on the Request passed in.
    Caller need not do so before passing in the request.

    This routine does not complete the request in case of failure.
    Caller must complete the request in case of failure.
*/

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS FORCEINLINE HfpSharedSendBrbSynchronously (_In_ WDFIOTARGET IoTarget, _In_ WDFREQUEST Request, _In_ PBRB Brb, _In_ ULONG BrbSize);




/*
 This routine formats a request with BRB and sends it asynchronously

 Arguments:
    IoTarget	- Target to send the BRB to
    Request		- request object to be formatted with BRB                
    Brb			- Brb to be sent
    BrbSize		- size of the Brb data structure
    ComplRoutine- WDF completion routine for the request
                  This must be specified because we are formatting the request and hence not using SEND_AND_FORGET flag
    Context		- (optional) context to be passed in to the completion routine

 Return Value:
    Success implies that request was sent correctly and completion routine will be called
    for it, failure implies it was not sent and caller should complete the request

Notes:
    This routine does not call WdfRequestReuse on the Request passed in.
    Caller must do so before passing in the request, if it is reusing the request.

    This routine does not complete the request in case of failure.
    Caller must complete the request in case of failure.
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS HfpSharedSendBrbAsync(
    _In_ WDFIOTARGET IoTarget,
    _In_ WDFREQUEST Request,
    _In_ PBRB Brb,
    _In_ size_t BrbSize,
    _In_ PFN_WDF_REQUEST_COMPLETION_ROUTINE ComplRoutine,
    _In_opt_ WDFCONTEXT Context
    );

