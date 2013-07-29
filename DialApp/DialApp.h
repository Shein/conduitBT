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
	cb - callback function pointer (see DialAppCb description)
 Exceptions: 
    throw int exception if an error happened.
 *************************************************************************************
 */
void  dialappInit (DialAppCb cb);


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
	After the normal completion the DialAppCb will be called with 
	state = DialAppState_DisconnectedDevicePresent, and correspondent bluetooth 
	device parameters set (see DialAppParam).
 *************************************************************************************
 */
void  dialappUiSelectDevice	();


/*
 *************************************************************************************
 This function causes to forget previously selected device. As result the State 
 Machine's current state should turn to DialAppState_IdleNoDevice.
 Exceptions: 
    No exceptions.
 Callback:
	After the normal completion the DialAppCb will be called with 
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
	pcsound    - PC sound = true/false - boolean flag indicating which sound device 
				 will be activated: PC or Phone sound.
 Exceptions: 
    No exceptions.
 Callback:
	If the call will successfully start, the DialAppCb will be called sequentially 
	with state = DialAppState_Calling, DialAppState_InCallPcSoundOff or 
	DialAppState_InCallPcSoundOn; otherwise the callback will report about errors.
 *************************************************************************************
 */
void  dialappCall (cchar* dialnumber, bool pcsound = true) throw();



/*
 *************************************************************************************
 Answer on the incoming call.
 Note: 
	This function may be called when the State Machine is in DialAppState_Ringing
	state only.
 Parameters:
	pcsound - PC sound = true/false - boolean flag indicating which sound device 
			  will be activated: PC or Phone sound.
 Exceptions: 
    No exceptions.
 Callback:
	If the call will successfully start, the DialAppCb will be called with 
	state = DialAppState_InCallPcSoundOff or DialAppState_InCallPcSoundOn; 
	otherwise the callback will report about errors.
 *************************************************************************************
 */
void  dialappAnswer (bool pcsound = true) throw();


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
void  dialappEndCall () throw();


/*
 *************************************************************************************
 Switch sound of the current call to PC or Phone.
 Note: 
	This function may be called when the State Machine is in 
	DialAppState_InCallPcSoundOff or DialAppState_InCallPcSoundOn states only.
 Parameters:
	pcsound - PC sound = true/false - boolean flag indicating which sound device 
			  will be activated: PC or Phone sound.
 Exceptions: 
    No exceptions.
 Callback:
	If the call will successfully start, the DialAppCb will be called with 
	state = DialAppState_InCallPcSoundOff or DialAppState_InCallPcSoundOn; 
	otherwise the callback will report about errors.
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


#endif // _DIALAPP_H
