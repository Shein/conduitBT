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
	DialAppDebug_DisablePnonePolling,	// Disable phone polling for automatic connection
	DialAppDebug_DisconnectNow,			// Disconnect phone (to test the polling)
	DialAppDebug_ConnectNow				// Connect phone (when disconnected)
};





/********************************************************************************************\
	  Public Functions (some functions throw exceptions of int type = DialAppError enum)
	  NOTE: use DIALAPP_DYN_USAGE definition in order to link with DialApp.dll dynamically
\********************************************************************************************/

#ifndef DIALAPP_DYN_USAGE

/*
 *************************************************************************************
 Initializes the DialApp application and Bluetooth HFP driver.
 During this function running or a immediately after it a callback with raised 
 DIALAPP_FLAG_INITSTATE will be called. It's the signal to external application that the 
 DialApp library is initialized and ready to serve other events. The callback may be only 
 not called in the case of error happening when the function is interrupted by an exception 
 bringing an error code. In this situation the external application must call dialappEnd()
 and stop interacting with the DialApp.

 Parameters:
	cb		- Callback function pointer (see DialAppCb description).
	pcsound - boolean flag for user's PC Sound switch initial preference (wish to use 
	          PS sound devices for voice). Further switch is performed via dialappPcSound func.
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
 Get list of paired devices. Caller should copy the devices strings in order to use 
 in other context.
 Exceptions: 
    throw int exception if an error happened.
 Callback:
	No callbacks.
 Returns:
	Number of elements in the 'DialAppBthDev* devices[]' array
 *************************************************************************************
 */
int dialappGetPairedDevices (DialAppBthDev* &devices);



/*
 *************************************************************************************
 Selects new bluetooth device to be a current. The supplied devaddr must be an address 
 of one previously paired devices (may be requested by dialappGetPairedDevices). If the 
 devaddr = 0 then the standard Windows Bluetooth device select and pair dialog will be 
 shown. Then if a user successfully completes pairing with new device, this device will 
 be set as a current. Then it should be connected and ready for usage.
 Exceptions: 
    throw int exception if an error happened.
 Parameters:
	devaddr - device MAC address; if 0 then new pairing dailog will be shown.
 Callback:
	After the normal completion, the DialAppCb will be called with 
	state = DialAppState_DisconnectedDevicePresent, and the correspondent bluetooth 
	device parameters set (see DialAppParam). Then, after successful connection the 
	subsequent callback should bring state = DialAppState_ServiceConnected.
	In the case of error the callback with correspondent state and error code 
	DialAppError will be called.
 *************************************************************************************
 */
void  dialappSelectDevice (uint64 devaddr = 0);


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
 Gets current device information structure (may be useful when the correspondent 
 callback (flags contains DIALAPP_FLAG_CURDEV) was not received by host application.
 If no current device, the return value is 0.
 Exceptions: 
    No exceptions.
 Callback:
	No callbacks.
 *************************************************************************************
 */
DialAppBthDev* dialappGetSelectedDevice () throw();


/*
 *************************************************************************************
 Gets State Machine's current state of DialAppState type (may be useful if callbacks 
 were not received)
 Exceptions: 
    No exceptions.
 Callback:
	No callbacks.
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
	with state = DialAppState_Calling, DialAppState_InCall; otherwise the callback 
	will report about errors.
	In the case of error the callback with correspondent state and error code 
	DialAppError will be called.
 *************************************************************************************
 */
void  dialappCall (cchar* dialnumber) throw();



/*
 *************************************************************************************
 Answer on an incoming or waiting call.
 Note: 
	This function may be called when the State Machine is in DialAppState_Ringing
	state or in DialAppState_InCall after receiving DIALAPP_FLAG_ABONENT_WAITING 
	notification callback.
 Exceptions: 
    No exceptions.
 Callback:
	If the call will successfully start, the DialAppCb will be called with 
	state = DialAppState_InCall, otherwise the callback will report about errors.
	This or subsequent callback will also notify about calling information changes
	by DIALAPP_FLAG_ABONENT_CURRENT / DIALAPP_FLAG_ABONENT_HELD / 
	DIALAPP_FLAG_ABONENT_WAITING flags.
	In the case of error the callback with correspondent state and error code 
	DialAppError will be called.
 *************************************************************************************
 */
void  dialappAnswer () throw();


/*
 *************************************************************************************
 Terminates current or cancels incoming call.
 Note: 
	This function may be called when the State Machine is in DialAppState_Calling, 
	DialAppState_Ringing, DialAppState_InCall states.
 Exceptions: 
    No exceptions.
 Callback:
	If the call will be successfully terminated the DialAppCb will be called with 
	state = DialAppState_ServiceConnected; otherwise the callback will report 
	about errors.
 *************************************************************************************
 */
void  dialappEndCall () throw();


/*
 *************************************************************************************
 Set user's preference to use or not PC Sound devices for future voice conversations. 
 If the phone is currently in conversation (in the DialAppState_InCall state), it also 
 immediately switches the current sound to the desired mode.
 Parameters:
	pcsound - PC Sound devices usage preference, boolean.
 Exceptions: 
    No exceptions.
 Callback:
	After switching to the desired mode the DialAppCb will be called for current state 
	and DialAppParam param->PcSoundPref value set. In the case a current state permits 
	to switch the PC sound on right now (DialAppState_InCall), and the switch succeeded,
	the DialAppParam param->PcSoundNow will be also set.
	In the case of error the callback with correspondent state and error code 
	DialAppError will be called.
 *************************************************************************************
 */
void  dialappPcSound (bool pcsound) throw();


/*
 *************************************************************************************
 Send DTMF character.
 Note: 
	This function may be called when the internal State Machine is in 
	DialAppState_InCall state only.
 Parameters:
	dialchar - DTMF character to send.
 Exceptions: 
    No exceptions.
 Callback:
	No special callback reporting about completion of this routine.
	In the case of error the callback with correspondent state and error code 
	DialAppError will be called.
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
	No callbacks.
 *************************************************************************************
 */
void dialappDebugMode (DialAppDebug debugtype, int mode = 0) throw();


/*
 *************************************************************************************
 Put current call on hold and to answer on another waiting call.
 Note: 
	This function may be called when the internal State Machine is in 
	DialAppState_InCall state only.
 Exceptions: 
	No exceptions.
 Callback:
	After switching to a waiting call, a callback with the state = DialAppState_InCall and 
	the flag combined from DIALAPP_FLAG_ABONENT_CURRENT, DIALAPP_FLAG_ABONENT_WAITING
	and DIALAPP_FLAG_ABONENT_HELD will be called.
 *************************************************************************************
 */
void dialappPutOnHold() throw();



/********************************************************************************************\
								Dynamic Linkage Support
\********************************************************************************************/
#else  // DIALAPP_DYN_USAGE is defined

#include <windows.h>


typedef void  	(*DIALAPPInit)				(DialAppCb cb, bool pcsound);
typedef void  	(*DIALAPPEnd)				();
typedef int 	(*DIALAPPGetPairedDevices)	(DialAppBthDev* &devices);
typedef void  	(*DIALAPPSelectDevice)		(uint64 devaddr);
typedef void  	(*DIALAPPForgetDevice)		() throw();
typedef DialAppBthDev* (*DIALAPPGetSelectedDevice) () throw();
typedef int	 	(*DIALAPPGetCurrentState)	() throw();
typedef void  	(*DIALAPPCall)				(cchar* dialnumber) throw();
typedef void  	(*DIALAPPAnswer)			() throw();
typedef void  	(*DIALAPPEndCall)			() throw();
typedef void  	(*DIALAPPPcSound)			(bool pcsound) throw();
typedef void 	(*DIALAPPSendDtmf)			(cchar dialchar) throw();
typedef void 	(*DIALAPPDebugMode)			(DialAppDebug debugtype, int mode) throw();
typedef void 	(*DIALAPPPutOnHold)			() throw();


extern DIALAPPInit 				_dialappInit;				
extern DIALAPPEnd				_dialappEnd;				
extern DIALAPPGetPairedDevices	_dialappGetPairedDevices;	
extern DIALAPPSelectDevice		_dialappSelectDevice;		
extern DIALAPPForgetDevice		_dialappForgetDevice;		
extern DIALAPPGetSelectedDevice _dialappGetSelectedDevice;	
extern DIALAPPGetCurrentState	_dialappGetCurrentState;	
extern DIALAPPCall				_dialappCall;				
extern DIALAPPAnswer			_dialappAnswer;				
extern DIALAPPEndCall			_dialappEndCall;			
extern DIALAPPPcSound			_dialappPcSound;			
extern DIALAPPSendDtmf			_dialappSendDtmf;			
extern DIALAPPDebugMode			_dialappDebugMode;			
extern DIALAPPPutOnHold			_dialappPutOnHold;


#define DIALAPP_LINKAGE_VARIABLES	\
		DIALAPPInit 				_dialappInit;					\
		DIALAPPEnd					_dialappEnd;					\
		DIALAPPGetPairedDevices		_dialappGetPairedDevices;		\
		DIALAPPSelectDevice			_dialappSelectDevice;			\
		DIALAPPForgetDevice			_dialappForgetDevice;			\
		DIALAPPGetSelectedDevice 	_dialappGetSelectedDevice;		\
		DIALAPPGetCurrentState		_dialappGetCurrentState;		\
		DIALAPPCall					_dialappCall;					\
		DIALAPPAnswer				_dialappAnswer;					\
		DIALAPPEndCall				_dialappEndCall;				\
		DIALAPPPcSound				_dialappPcSound;				\
		DIALAPPSendDtmf				_dialappSendDtmf;				\
		DIALAPPDebugMode			_dialappDebugMode;				\
		DIALAPPPutOnHold			_dialappPutOnHold


inline void dialappInit (DialAppCb cb, bool pcsound = true)
{
	static HINSTANCE instDialapp = LoadLibraryA("DialApp.dll");
	if (!instDialapp)
		throw int(DialAppError_DllLoadError);

	_dialappInit 				= (DIALAPPInit) 			GetProcAddress (instDialapp, "dialappInit");
	_dialappEnd 				= (DIALAPPEnd) 				GetProcAddress (instDialapp, "dialappEnd");
	_dialappGetPairedDevices 	= (DIALAPPGetPairedDevices) GetProcAddress (instDialapp, "dialappGetPairedDevices");
	_dialappSelectDevice 		= (DIALAPPSelectDevice) 	GetProcAddress (instDialapp, "dialappSelectDevice");
	_dialappForgetDevice 		= (DIALAPPForgetDevice) 	GetProcAddress (instDialapp, "dialappForgetDevice");
	_dialappGetSelectedDevice 	= (DIALAPPGetSelectedDevice)GetProcAddress (instDialapp, "dialappGe");
	_dialappGetCurrentState 	= (DIALAPPGetCurrentState) 	GetProcAddress (instDialapp, "dialappGetCurrentState");
	_dialappCall 				= (DIALAPPCall) 			GetProcAddress (instDialapp, "dialappCall");
	_dialappAnswer 				= (DIALAPPAnswer) 			GetProcAddress (instDialapp, "dialappAnswer");
	_dialappEndCall 			= (DIALAPPEndCall) 			GetProcAddress (instDialapp, "dialappEndCall");
	_dialappPcSound 			= (DIALAPPPcSound) 			GetProcAddress (instDialapp, "dialappPcSound");
	_dialappSendDtmf 			= (DIALAPPSendDtmf) 		GetProcAddress (instDialapp, "dialappSendDtmf");
	_dialappDebugMode 			= (DIALAPPDebugMode) 		GetProcAddress (instDialapp, "dialappDebugMode");
	_dialappPutOnHold 			= (DIALAPPPutOnHold) 		GetProcAddress (instDialapp, "dialappPutOnHold");

	_dialappInit(cb,pcsound);
}

inline void  dialappEnd ()
{
	_dialappEnd();
}

inline int dialappGetPairedDevices (DialAppBthDev* &devices)
{
	return _dialappGetPairedDevices(devices);
}

inline void  dialappSelectDevice (uint64 devaddr = 0)
{
	_dialappSelectDevice(devaddr);
}

inline void  dialappForgetDevice () throw()
{
	_dialappForgetDevice();
}

inline DialAppBthDev* dialappGetSelectedDevice () throw()
{
	return _dialappGetSelectedDevice();
}

inline int	 dialappGetCurrentState () throw()
{
	return _dialappGetCurrentState();
}

inline void  dialappCall (cchar* dialnumber) throw()
{
	_dialappCall(dialnumber);
}

inline void  dialappAnswer () throw()
{
	_dialappAnswer();
}

inline void  dialappEndCall () throw()
{
	_dialappEndCall();
}

inline void  dialappPcSound (bool pcsound) throw()
{
	_dialappPcSound(pcsound);
}

inline void dialappSendDtmf (cchar dialchar) throw()
{
	_dialappSendDtmf(dialchar);
}

inline void dialappDebugMode (DialAppDebug debugtype, int mode = 0) throw()
{
	_dialappDebugMode(debugtype, mode);
}

inline void dialappPutOnHold() throw()
{
	_dialappPutOnHold();
}


#endif	// DIALAPP_DYN_USAGE



#endif // _DIALAPP_H
