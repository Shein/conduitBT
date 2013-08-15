/*++

Module Name:
    Sdp.c

Abstract:
    Contains functionality for publishing and freeing SDP record
    
Environment:
    Kernel mode only
--*/

#include "clisrv.h"
#include "sdp.h"

#if defined(EVENT_TRACING)
#include "sdp.tmh"
#endif


NTSTATUS
AddSeqAttribute(
    _In_ PBTHDDI_SDP_NODE_INTERFACE SdpNodeInterface,
    _In_ PSDP_TREE_ROOT_NODE Tree,
    _In_ USHORT AttribId,
    _Out_ PSDP_NODE * Seq
    );

NTSTATUS
AppendSeqNode(
    _In_ PBTHDDI_SDP_NODE_INTERFACE SdpNodeInterface,
    _In_ PSDP_NODE containerNode,
    _Out_ PSDP_NODE * Node
    );

NTSTATUS
AppendNodeUint16(
    _In_ PBTHDDI_SDP_NODE_INTERFACE SdpNodeInterface,
    _In_ PSDP_NODE ContainerNode,
    _In_ UINT16 Value
    );

NTSTATUS
AppendNodeUuid128(
    _In_ PBTHDDI_SDP_NODE_INTERFACE SdpNodeInterface,
    _In_ PSDP_NODE ContainerNode,
    _In_ const GUID * Value
    );

NTSTATUS
AppendNodeUuid16(
    _In_ PBTHDDI_SDP_NODE_INTERFACE SdpNodeInterface,
    _In_ PSDP_NODE ContainerNode,
    _In_ UINT16 Value
    );




NTSTATUS
AddSeqAttribute(
    _In_ PBTHDDI_SDP_NODE_INTERFACE SdpNodeInterface,
    _In_ PSDP_TREE_ROOT_NODE Tree,
    _In_ USHORT AttribId,
    _Out_ PSDP_NODE * Seq
    )
{
    NTSTATUS status;
    PSDP_NODE seq; 

    seq = SdpNodeInterface->SdpCreateNodeSequence(POOLTAG_HFPDRIVER);
    
    if (!seq) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TraceEvents(TRACE_LEVEL_ERROR, DBG_SDP, "Creating sequence failed, returning status code %d\n", status);
        goto exit;
    }

    status = SdpNodeInterface->SdpAddAttributeToTree(Tree, AttribId, seq, POOLTAG_HFPDRIVER);
    if(!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_SDP, "SdpAddAttributeToTree failed, Status=%X", status);
        // If we failed to add attribute to tree use ExFreePool to free it
        ExFreePool(seq);
        goto exit;
    }

    *Seq = seq;
exit:
    return status;
}


NTSTATUS
AppendSeqNode(
    _In_ PBTHDDI_SDP_NODE_INTERFACE SdpNodeInterface,
    _In_ PSDP_NODE containerNode,
    _Out_ PSDP_NODE * Node
    )
{
    NTSTATUS status;
    PSDP_NODE node;

    node = SdpNodeInterface->SdpCreateNodeSequence(POOLTAG_HFPDRIVER);
    if(!node) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TraceEvents(TRACE_LEVEL_ERROR, DBG_SDP, "SdpCreateNodeSequence failed, returning status code %d\n", status);
        goto exit;
    }
    
    status = SdpNodeInterface->SdpAppendNodeToContainerNode(containerNode, node);

    // SdpAppendNodeToContainerNode doesn't fail due to resource reasons

    NT_ASSERT(NT_SUCCESS(status));

    *Node = node;
exit:
    return status;
}


NTSTATUS
AppendNodeUint16(
    _In_ PBTHDDI_SDP_NODE_INTERFACE SdpNodeInterface,
    _In_ PSDP_NODE ContainerNode,
    _In_ UINT16 Value
    )
{
    NTSTATUS status;
    PSDP_NODE node;
    
    node = SdpNodeInterface->SdpCreateNodeUint16(Value, POOLTAG_HFPDRIVER);
    if(!node) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TraceEvents(TRACE_LEVEL_ERROR, DBG_SDP, "Creating NodeUint16 failed, returning status code %d\n", status);
        goto exit;
    }

    status = SdpNodeInterface->SdpAppendNodeToContainerNode(ContainerNode, node);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_SDP, "SdpAppendNodeToContainerNode failed, returning status code %d\n", status);
        ExFreePool(node);
        node = NULL;
        goto exit;
    }

    NT_ASSERT(NT_SUCCESS(status));

exit:
    return status;
}


NTSTATUS
AppendNodeUuid128(
    _In_ PBTHDDI_SDP_NODE_INTERFACE SdpNodeInterface,
    _In_ PSDP_NODE ContainerNode,
    _In_ const GUID * Value
    )
{
    NTSTATUS status;
    PSDP_NODE node;
    
    node = SdpNodeInterface->SdpCreateNodeUuid128(Value, POOLTAG_HFPDRIVER);
    if(!node) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TraceEvents(TRACE_LEVEL_ERROR, DBG_SDP, "Creating NodeUuid128 failed, returning status code %d\n", status);
        goto exit;
    }

    status = SdpNodeInterface->SdpAppendNodeToContainerNode (ContainerNode, node);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_SDP, "SdpAppendNodeToContainerNode failed, returning status code %d\n", status);
        ExFreePool(node);
        node = NULL;
        goto exit;
    }

    NT_ASSERT(NT_SUCCESS(status));

exit:
    return status;
}


NTSTATUS
AppendNodeUuid16(
    _In_ PBTHDDI_SDP_NODE_INTERFACE SdpNodeInterface,
    _In_ PSDP_NODE ContainerNode,
    _In_ UINT16 Value
    )
{
    NTSTATUS status;
    PSDP_NODE node;
    
    node = SdpNodeInterface->SdpCreateNodeUuid16(Value, POOLTAG_HFPDRIVER);
    if(!node) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TraceEvents(TRACE_LEVEL_ERROR, DBG_SDP, "Creating NodeUuid16 failed, returning status code %d\n", status);
        goto exit;
    }

    status = SdpNodeInterface->SdpAppendNodeToContainerNode (ContainerNode, node);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_SDP, "SdpAppendNodeToContainerNode failed, returning status code %d\n", status);
        ExFreePool(node);
        node = NULL;
        goto exit;
    }

    NT_ASSERT(NT_SUCCESS(status));

exit:
    return status;
}




NTSTATUS
CreateSdpRecord(
    _In_ PBTHDDI_SDP_NODE_INTERFACE SdpNodeInterface,
    _In_ PBTHDDI_SDP_PARSE_INTERFACE SdpParseInterface,
    _In_ UINT16 ClassId,
    _In_ LPWSTR Name,
    _Out_ PUCHAR * Stream,
    _Out_ ULONG * Size
    )
{
    NTSTATUS status;
    PSDP_TREE_ROOT_NODE tree = NULL;
    PSDP_NODE seqClsIdList;
    PSDP_NODE nodeName = NULL, nodeDesc = NULL; 
    UNICODE_STRING unicodeStrName;
    ANSI_STRING ansiStrName;
    PUCHAR stream = NULL;
    ULONG size;
    ULONG_PTR errorByte = 0;

    RtlInitUnicodeString(&unicodeStrName, Name);
    RtlInitAnsiString(&ansiStrName, NULL);

    status = RtlUnicodeStringToAnsiString(&ansiStrName, &unicodeStrName, TRUE);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_SDP, "Creating ANSI string for service name failed, Status=%X", status);
        ansiStrName.Length = 0;
        goto exit;
    }
        
    tree = SdpNodeInterface->SdpCreateNodeTree (POOLTAG_HFPDRIVER);
    if (!tree) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TraceEvents(TRACE_LEVEL_ERROR, DBG_SDP, "SdpCreateNodeTree failed, returning Status=%X", status);
        goto exit;
    }

    //
    // Add ClassIdList attribute
    //

    status = AddSeqAttribute(SdpNodeInterface, tree, SDP_ATTRIB_CLASS_ID_LIST, &seqClsIdList);
    if (!NT_SUCCESS(status))
        goto exit;

    status = AppendNodeUuid16(SdpNodeInterface, seqClsIdList, ClassId);
    if (!NT_SUCCESS(status))
        goto exit;

    //
    // Add protocols
    //

    //
    // L2CAP
    //
	//		status = AddSeqAttribute(SdpNodeInterface, tree, SDP_ATTRIB_PROTOCOL_DESCRIPTOR_LIST, &seqProto);
	//		if (!NT_SUCCESS(status))
	//		    goto exit;
	//		
	//		status = AppendSeqNode(SdpNodeInterface, seqProto, &nodeProto);
	//		if (!NT_SUCCESS(status))
	//		    goto exit;
	//		
	//		status = AppendNodeUuid16(SdpNodeInterface, nodeProto, L2CAP_PROTOCOL_UUID16);
	//		if (!NT_SUCCESS(status))
	//		    goto exit;

	//		status = AppendNodeUint16(SdpNodeInterface, nodeProto, Psm);
	//		if (!NT_SUCCESS(status))
	//		    goto exit;

    //
    // SDP
    //

	//     status = AppendSeqNode(SdpNodeInterface, seqProto, &nodeProto);
	//     if (!NT_SUCCESS(status))
	//         goto exit;
	// 
	//     status = AppendNodeUuid16(SdpNodeInterface, nodeProto, SDP_PROTOCOL_UUID16);            
	//     if (!NT_SUCCESS(status))
	//         goto exit;

    
    //
    // Add service name
    //

    nodeName = SdpNodeInterface->SdpCreateNodeString (ansiStrName.Buffer, ansiStrName.Length, POOLTAG_HFPDRIVER);
    if (!nodeName) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TraceEvents(TRACE_LEVEL_ERROR, DBG_SDP, "Creating node for service name failed, Status=%X", status);
        goto exit;
    }
    
    status = SdpNodeInterface->SdpAddAttributeToTree (tree, LANG_DEFAULT_ID+STRING_NAME_OFFSET, nodeName, POOLTAG_HFPDRIVER);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_SDP,  "SdpAddAttributeToTree for service name failed, Status=%X", status);
        goto exit;
    }

    nodeName = NULL; //transferred ownership to tree
    nodeDesc = SdpNodeInterface->SdpCreateNodeString (ansiStrName.Buffer, ansiStrName.Length, POOLTAG_HFPDRIVER);

    if (!nodeDesc) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TraceEvents(TRACE_LEVEL_ERROR, DBG_SDP,  "Creating node for service desc failed, Status=%X", status);
        goto exit;
    }
    
    status = SdpNodeInterface->SdpAddAttributeToTree (tree, LANG_DEFAULT_ID+STRING_DESCRIPTION_OFFSET, nodeDesc, POOLTAG_HFPDRIVER);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_SDP, "SdpAddAttributeToTree for service desc failed, Status=%X", status);
        goto exit;
    }

    nodeDesc = NULL;

    // Create stream from tree

    status = SdpParseInterface->SdpConvertTreeToStream(tree, &stream, &size, POOLTAG_HFPDRIVER);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_SDP, "Failed to get stream from tree for SDP record, Status=%X", status);
        goto exit;
    }

    status = SdpParseInterface->SdpValidateStream (stream, size, &errorByte);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_SDP, "Validate stream failed for SDP record, first failure at address %p\n", (void*)errorByte);
        goto exit;
    }

    *Stream = stream;
    *Size = size;
    
exit:
    if (tree)
        SdpNodeInterface->SdpFreeTree(tree);

    if (nodeName) {
        // If we failed to add attribute to tree use ExFreePool to free it
        ExFreePool(nodeName);
    }
    
    if (nodeDesc) {
        // If we failed to add attribute to tree use ExFreePool to free it
        ExFreePool(nodeDesc);
    }
    
    if (ansiStrName.Length)
        RtlFreeAnsiString(&ansiStrName);

    if (!NT_SUCCESS(status)) {
        if (stream != NULL)
            ExFreePoolWithTag(stream, POOLTAG_HFPDRIVER);
    }
    
    return status;
}


void FreeSdpRecord(PUCHAR SdpRecord)
{
    ExFreePoolWithTag(SdpRecord, POOLTAG_HFPDRIVER);            
}

