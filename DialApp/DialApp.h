/********************************************************************************************\
 Library     :  HFP DialApp (Bluetooth HFP library)
 Filename    :  DialApp.h
 Purpose     :  Public h-file for DialApp.dll (together with DialAppType.h)
 Note		 :  All functions may throw C++ exceptions of integer type (DialAppError)
\********************************************************************************************/

#ifndef _DIALAPP_H
#define _DIALAPP_H


#include "DialAppType.h"


/*
 *************************************************************************************
 dialappDebugMode function cases
 *************************************************************************************
 */
enum DialAppDebug
{
	DialAppDebug_HfpAtCommands,			// Disable HFP negotiation (for some phones)
	DialAppDebug_DisablePnonePolling,	// Disable phone polling for automatic connection
	DialAppDebug_DisconnectNow,			// Disconnect phone (to test the polling)
	DialAppDebug_ConnectNow				// Connect phone (when disconnected)
};





/********************************************************************************************\
		Public Functions (some functions throw exceptions of int type = DialAppError enum)
\********************************************************************************************/


/*
 *************************************************************************************
 Initializes the DialApp application and Bluetooth HFP driver.
 Parameters:
	cb		- Callback function pointer (see DialAppCb description).
	pcsound - boolean flag for PC sound. This is initial state for the desired sound 
			  device. Further switch is performed via dialappPcSound func.
 Exceptions: 
    throw int exception if an error happened.
 *************************************************************************************
 */
void  dialappInit (DialAppCb cb, bool pcsound = true);


/*
 *************************************************************************************
 Finalizes the DialApp application and closes the driver.
 Exceptions: 
    throw int exception if an error happened.
 *************************************************************************************
 */
void  dialappEnd ();


/*
 *************************************************************************************
 Opens standard Windows Bluetooth device select and pair dialog. If a user choses 
 correct device and pairs with it, this device will be set as current.
 Exceptions: 
    throw int exception if an error happened.
 Callback:
	After the normal completion, the DialAppCb will be called with 
	state = DialAppState_DisconnectedDevicePresent, and the correspondent bluetooth 
	device parameters set (see DialAppParam).
 *************************************************************************************
 */
void  dialappUiSelectDevice	();


/*
 *************************************************************************************
 Get list of paired devices. Caller should copy the devices strings in order to use 
 in other contexts.
 Exceptions: 
    throw int exception if an error happened.
 Callback:
	No.
 Returns:
	Number of wide-char strings in the 'wchar* devices[]' array
 *************************************************************************************
 */
int dialappGetPairedDevices(wchar** (&devices));



/*
 *************************************************************************************
 This function causes to forget previously selected device. As result the State 
 Machine's current state should turn to DialAppState_IdleNoDevice.
 Exceptions: 
    No exceptions.
 Callback:
	After the normal completion, the DialAppCb will be called with 
	state = DialAppState_IdleNoDevice
 *************************************************************************************
 */
void  dialappForgetDevice () throw();


/*
 *************************************************************************************
 Gets current device name (may be useful when the correspondent callback 
 (DialAppState_DisconnectedDevicePresent) was not received by host application.
 Exceptions: 
    No exceptions.
 Callback:
	No callback.
 *************************************************************************************
 */
wchar* dialappGetSelectedDevice () throw();


/*
 *************************************************************************************
 Gets State Machine's current state of DialAppState type (may be useful if callbacks 
 were not received)
 Exceptions: 
    No exceptions.
 Callback:
	No callback.
 *************************************************************************************
 */
int	 dialappGetCurrentState () throw();


/*
 *************************************************************************************
 Starts outgoing call.
 Note: 
	This function may be called when the State Machine is in 
	DialAppState_ServiceConnected state only.
 Parameters:
	dialnumber - pointer to ANSI string dialing number (the string buffer may be 
				 temporal, the string content will be copied)
 Exceptions: 
    No exceptions.
 Callback:
	If the call will successfully start, the DialAppCb will be called sequentially 
	with state = DialAppState_Calling, DialAppState_InCallPcSoundOff or 
	DialAppState_InCallPcSoundOn; otherwise the callback will report about errors.
 *************************************************************************************
 */
void  dialappCall (cchar* dialnumber) throw();



/*
 *************************************************************************************
 Answer on the incoming call.
 Note: 
	This function may be called when the State Machine is in DialAppState_Ringing
	state only.
 Exceptions: 
    No exceptions.
 Callback:
	If the call will successfully start, the DialAppCb will be called with 
	state = DialAppState_InCallPcSoundOff or DialAppState_InCallPcSoundOn; 
	otherwise the callback will report about errors.
 *************************************************************************************
 */
void  dialappAnswer () throw();


/*
 *************************************************************************************
 Terminates current or cancels incoming call.
 Note: 
	This function may be called when the State Machine is in DialAppState_Calling, 
	DialAppState_Ringing, DialAppState_InCallPcSoundOff or DialAppState_InCallPcSoundOn
	states.
 Exceptions: 
    No exceptions.
 Callback:
	If the call will be successfully terminated the DialAppCb will be called with 
	state = DialAppState_ServiceConnected;
	otherwise the callback will report about errors.
 *************************************************************************************
 */
void  dialappEndCall (int callID = 0) throw();


/*
 *************************************************************************************
 Switch sound of the current or next call(s) to PC or Phone.
 Parameters:
	pcsound - boolean flag for PC sound. May be chosen when needed .
 Exceptions: 
    No exceptions.
 Callback:
	If a call exists, the DialAppCb will be called with 
	state = DialAppState_InCallPcSoundOff or DialAppState_InCallPcSoundOn; 
	otherwise this callback will bring a new value in 3d DialAppParam param.
 *************************************************************************************
 */
void  dialappPcSound (bool pcsound) throw();


/*
 *************************************************************************************
 Send DTMF character.
 Note: 
	This function may be called when the State Machine is in 
	DialAppState_InCallPcSoundOff or DialAppState_InCallPcSoundOn states only.
 Parameters:
	dialchar - DTMF character to send.
 Exceptions: 
    No exceptions.
 Callback:
	No callback.
 *************************************************************************************
 */
void dialappSendDtmf (cchar dialchar) throw();


/*
 *************************************************************************************
 Special function used for debug purpose.
 Parameters:
	debugtype - debug parameter type (see DialAppDebug).
	mode	  - new mode or value
 Exceptions: 
    No exceptions.
 Callback:
	No callback.
 *************************************************************************************
 */
void dialappDebugMode (DialAppDebug debugtype, int mode = 0) throw();


/*
 *************************************************************************************
 Put current call on hold, to answer another.
 Note: 
	This function may be called when the State Machine is in 
	DialAppState_InCallPcSoundOff or DialAppState_InCallPcSoundOn states only.
 Parameters:
	TBD
 Exceptions: 
    TBD
 Callback:
	Returns CallID, in DialAppParam (3rd param). The field param - TBD
 *************************************************************************************
 */
void dialappPutOnHold() throw();


/*
 *************************************************************************************
 Activate held call.
 Note: 
	This function may be called when the State Machine is in 
	DialAppState_InCallPcSoundOff or DialAppState_InCallPcSoundOn states only.
 Parameters:
	int callid - to be received from the AG:   AT+CIEV:< CallHeld Indicator >,2
 Exceptions: 
    TBD
 Callback:
	TBD
 *************************************************************************************
 */
void dialappActivateHeldCall(int callid) throw();


void dialappSendAT(char *at);


#endif // _DIALAPP_H
