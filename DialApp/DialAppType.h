/********************************************************************************************\
 Library     :  HFP DialApp (Bluetooth HFP library)
 Filename    :  DialAppType.h
 Purpose     :  Public h-file for DialApp.dll - types information
\********************************************************************************************/

#ifndef _DIALAPPTYPES_H
#define _DIALAPPTYPES_H


/********************************************************************************************\
						Common types (duplicated from def.h)
\********************************************************************************************/

typedef const char			cchar;
typedef const wchar_t		wchar;
typedef unsigned long long	uint64;



/********************************************************************************************\
								Public data types
\********************************************************************************************/

/*
 *************************************************************************************
 DialApp error codes.
 May be thrown as integer C++ exception from all functions.
 *************************************************************************************
 */
enum DialAppError
{
	// Common errors
	DialAppError_Ok							,
	DialAppError_InternalError				,
	DialAppError_InsufficientResources		,

	// Init errors
	DialAppError_InitBluetoothRadioError	,
	DialAppError_InitDriverError			,

	// HFP API errors
	DialAppError_UnknownDevice				,
	DialAppError_IncorrectState4Call		,
	DialAppError_ConnectFailure				,
	DialAppError_ServiceConnectFailure		,
	DialAppError_CallFailure				,

	// Audio channel errors
	DialAppError_OpenScoFailure				,
	DialAppError_ReadWriteScoError			,
	DialAppError_WaveApiError				,
};


/*
 *************************************************************************************
 DialApp states, passed to a host application via DialAppCb each time the state 
 is changed. Also, dialappGetCurState func gets this enum values.
 *************************************************************************************
 */
enum DialAppState
{
	DialAppState_IdleNoDevice,
	DialAppState_DisconnectedDevicePresent,
	DialAppState_Disconnecting,
	DialAppState_Connecting,
	DialAppState_Connected,
	DialAppState_ServiceConnecting,
	DialAppState_ServiceConnected,
	DialAppState_Calling,
	DialAppState_Ringing,
	DialAppState_InCallHeadsetOff,
	DialAppState_InCallHeadsetOn
};


/*
 *************************************************************************************
 DialApp debug types.
 *************************************************************************************
 */
enum DialAppDebug
{
	DialAppDebug_HfpAtCommands,
};


union DialAppParam
{
	struct {
		uint64  BthAddr;
		wchar  *BthName;
	};
	wchar * Abonent;
};

/*
 *************************************************************************************
 DialApp callback function prototype. The correspondent function should be passed to 
 dialappInit and will receive notifications about all device state changes and 
 asynchronous errors.
 TBD
 *************************************************************************************
 */
typedef void (*DialAppCb) (DialAppState state, DialAppError status, DialAppParam* param);



#endif // _DIALAPPTYPES_H
