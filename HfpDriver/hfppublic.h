/*++

ModuleName:
    hfppublic.h

Abstract:
    Contains definitions used both by driver and application
--*/

#pragma once


#define POOLTAG_HFPDRIVER 'htbw'


/*
  Device interface exposed by our bth client device
*/

/* HFP Profile Service Class: 0000111E-0000-1000-8000-00805F9B34FB */
//DEFINE_GUID(HFP_DEVICE_INTERFACE, 0x0000111E, 0x0000, 0x1000, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB);

#define HFP_CLASS_ID	0x111E

/* fc71b33d-d528-4763-a86c-78777c7bcd7b */
DEFINE_GUID(HFP_DEVICE_INTERFACE, 0xfc71b33d, 0xd528, 0x4763, 0xa8, 0x6c, 0x78, 0x77, 0x7c, 0x7b, 0xcd, 0x7b);

extern __declspec(selectany) const PWSTR BthHfpDriverName = L"HfpDriver";

/*
  IOCTL_xxx codes definitions for our device.
  Let's use FILE_DEVICE_TRANSPORT type for IOCTL, it's taken from ntddk.h
  Functions 2048-4095 are reserved for OEMs
*/

#ifndef FILE_DEVICE_TRANSPORT
#define FILE_DEVICE_TRANSPORT   0x21
#endif


typedef struct
{
	UINT64		DestAddr;				// Destination Bluetooth device
	UINT64		EvHandleScoConnect;		// User-mode Event handle for SCO Connect
	UINT64		EvHandleScoDisconnect;	// User-mode Event handle for SCO Disconnect
	UINT64		EvHandleScoCritError;	// User-mode Event handle for SCO Critical error
	BOOLEAN		ConnectReadiness;		// HFPDEVICE_CONTEXT::ConnectReadiness init value
} HFP_REG_SERVER;


#define IOCTL_HFP_REG_SERVER			CTL_CODE (FILE_DEVICE_TRANSPORT, 2048, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_HFP_UNREG_SERVER			CTL_CODE (FILE_DEVICE_TRANSPORT, 2049, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_HFP_OPEN_SCO				CTL_CODE (FILE_DEVICE_TRANSPORT, 2050, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_HFP_CLOSE_SCO				CTL_CODE (FILE_DEVICE_TRANSPORT, 2051, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_HFP_INCOMING_READINESS	CTL_CODE (FILE_DEVICE_TRANSPORT, 2052, METHOD_BUFFERED, FILE_ANY_ACCESS)
