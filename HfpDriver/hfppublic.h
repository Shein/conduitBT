/*++

ModuleName:
    hfppublic.h

Abstract:
    Contains definitions used both by driver and application
--*/

#pragma once

/*
  Device interface exposed by our bth client device
*/

/* fc71b33d-d528-4763-a86c-78777c7bcd7b */
DEFINE_GUID(HFP_DEVICE_INTERFACE, 0xfc71b33d, 0xd528, 0x4763, 0xa8, 0x6c, 0x78, 0x77, 0x7c, 0x7b, 0xcd, 0x7b);

/*
  IOCTL_xxx codes definitions for our device.
  Let's use FILE_DEVICE_TRANSPORT type for IOCTL, it's taken from ntddk.h
  Functions 2048-4095 are reserved for OEMs
*/

#ifndef FILE_DEVICE_TRANSPORT
#define FILE_DEVICE_TRANSPORT   0x21
#endif

#define IOCTL_HFP_OPEN_SCO		CTL_CODE (FILE_DEVICE_TRANSPORT, 2048, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_HFP_CLOSE_SCO		CTL_CODE (FILE_DEVICE_TRANSPORT, 2049, METHOD_BUFFERED, FILE_ANY_ACCESS)
