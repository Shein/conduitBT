/*++

Module Name:
    Sdp.h

Abstract:
    Contains declarations for SDP record related functions
    
Environment:
    Kernel mode only
--*/




/*
 Create server SDP record

 Arguments:
    SdpNodeInterface	- Node interface that we obtained from bth stack
    SdpParseInterface	- Parse interface that we obtained from bth stack
    ClassId				- Service Class ID to publish
    Name				- Service name to publish
    Stream				- receives the SDP record stream
    Size				- receives size of SDP record stream

 Return Value:
    NTSTATUS Status code.
*/
NTSTATUS
CreateSdpRecord(
    _In_ PBTHDDI_SDP_NODE_INTERFACE SdpNodeInterface,
    _In_ PBTHDDI_SDP_PARSE_INTERFACE SdpParseInterface,
    _In_ UINT16 ClassId,
    _In_ LPWSTR Name,
    _Out_ PUCHAR * Stream,
    _Out_ ULONG * Size
    );


void FreeSdpRecord (PUCHAR SdpRecord);

