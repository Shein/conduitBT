/********************************************************************************\
 Filename    :  HfpSm.cpp
 Purpose     :  Main HFP State Machine
\********************************************************************************/

#include "def.h"
#include "deblog.h"
#include "HfpSm.h"
#include "smBody.h"


IMPL_STATES (HfpSm, STATE_LIST_HFPSM)

HfpSm		HfpSmObj;
HfpSmCb  	HfpSm::UserCallback;


void HfpSm::Construct()
{
	SMT<HfpSm>::Construct (SM_HFP);
	ScoAppObj = new ScoApp();
	ScoAppObj->Construct();
}


void HfpSm::Destruct()
{
	delete ScoAppObj;
}


void HfpSm::Init (DialAppCb cb)
{
    /*---------------------------------- STATE: Idle ---------------------------------------------------*/
    InitStateNode (STATE_Idle,				SMEV_SelectDevice,				STATE_Disconnected,		&SelectDevice);
    InitStateNode (STATE_Idle,				SMEV_ForgetDevice,				STATE_Idle,				NULLTRANS);
    InitStateNode (STATE_Idle,				SMEV_StartOutgoingCall,			STATE_Idle,				&IncorrectState4Call);
    /*-------------------------------------------------------------------------------------------------*/

    /*---------------------------------- STATE: Disconnected ------------------------------------------*/
    InitStateNode (STATE_Disconnected,		SMEV_Failure,					STATE_Idle,				NULLTRANS);
    InitStateNode (STATE_Disconnected,		SMEV_ForgetDevice,				STATE_Idle,				&ForgetDevice);
    InitStateNode (STATE_Disconnected,		SMEV_SelectDevice,				STATE_Disconnected,		&SelectDevice);
    InitStateNode (STATE_Disconnected,		SMEV_ConnectStart,				STATE_Connecting,		&Connect);
    InitStateNode (STATE_Disconnected,		SMEV_StartOutgoingCall,			STATE_Disconnected,		&IncorrectState4Call);
    /*-------------------------------------------------------------------------------------------------*/

    /*---------------------------------- STATE: Connecting   ------------------------------------------*/
    InitStateNode (STATE_Connecting,		SMEV_ForgetDevice,				STATE_Idle,				&ForgetDevice);
    InitStateNode (STATE_Connecting,		SMEV_SelectDevice,				STATE_Disconnected,		&SelectDevice);
    InitStateNode (STATE_Connecting,		SMEV_Failure,					STATE_Disconnected,		&ConnectFailure);
    InitStateNode (STATE_Connecting,		SMEV_Connected,					STATE_Connected,		&Connected);
    InitStateNode (STATE_Connecting,		SMEV_StartOutgoingCall,			STATE_Connecting,		&IncorrectState4Call);
    /*-------------------------------------------------------------------------------------------------*/

    /*---------------------------------- STATE: Connected   -------------------------------------------*/
    InitStateNode (STATE_Connected,			SMEV_ForgetDevice,				STATE_Idle,				&ForgetDevice);
    InitStateNode (STATE_Connected,			SMEV_SelectDevice,				STATE_Disconnected,		&SelectDevice);
    InitStateNode (STATE_Connected,			SMEV_DisconnectStart,			STATE_Disconnecting,	NULLTRANS);
    InitStateNode (STATE_Connected,			SMEV_HfpConnectStart,			STATE_HfpConnecting,	&HfpConnect);
    InitStateNode (STATE_Connected,			SMEV_StartOutgoingCall,			STATE_Connected,		&IncorrectState4Call);
    /*-------------------------------------------------------------------------------------------------*/

    /*---------------------------------- STATE: HfpConnecting   ---------------------------------------*/
    InitStateNode (STATE_HfpConnecting,		SMEV_ForgetDevice,				STATE_Idle,				&ForgetDevice);
    InitStateNode (STATE_HfpConnecting,		SMEV_SelectDevice,				STATE_Disconnected,		&SelectDevice);
    InitStateNode (STATE_HfpConnecting,		SMEV_Failure,					STATE_Disconnected,		&ServiceConnectFailure);
    InitStateNode (STATE_HfpConnecting,		SMEV_HfpConnected,				STATE_HfpConnected,		&HfpConnected);
    InitStateNode (STATE_HfpConnecting,		SMEV_StartOutgoingCall,			STATE_HfpConnecting,	&IncorrectState4Call);
    /*------------------------------------------------------------------------------------------------*/

    /*---------------------------------- STATE: HfpConnected   ----------------------------------------*/
    InitStateNode (STATE_HfpConnected,		SMEV_ForgetDevice,				STATE_Idle,				&ForgetDevice);
    InitStateNode (STATE_HfpConnected,		SMEV_SelectDevice,				STATE_Disconnected,		&SelectDevice);
    InitStateNode (STATE_HfpConnected,		SMEV_DisconnectStart,			STATE_Disconnecting,	NULLTRANS);
    InitStateNode (STATE_HfpConnected,		SMEV_EndCall,					STATE_HfpConnected,		NULLTRANS);
    InitStateNode (STATE_HfpConnected,		SMEV_IncomingCall,				STATE_Ringing,			&Ringing);
    InitStateNode (STATE_HfpConnected,		SMEV_StartOutgoingCall,			STATE_Calling,			&OutgoingCall);
    InitStateNode (STATE_HfpConnected,		SMEV_AbonentInfo,				STATE_HfpConnected,		&AbonentInfo);
    /*-------------------------------------------------------------------------------------------------*/

    /*---------------------------------- STATE: Calling   ---------------------------------------------*/
    InitStateNode (STATE_Calling,			SMEV_Failure,					STATE_HfpConnected,		&CallFailure);
    InitStateNode (STATE_Calling,			SMEV_EndCall,					STATE_HfpConnected,		&EndCall);
    InitStateNode (STATE_Calling,			SMEV_CallSetup,					STATE_InCallHeadsetOff,	&StartCall);
    /*-------------------------------------------------------------------------------------------------*/

    /*---------------------------------- STATE: Ringing   ---------------------------------------------*/
    InitStateNode (STATE_Ringing,			SMEV_Failure,					STATE_HfpConnected,		&EndCall);
    InitStateNode (STATE_Ringing,			SMEV_EndCall,					STATE_HfpConnected,		&EndCall);
    InitStateNode (STATE_Ringing,			SMEV_Answer,					STATE_InCallHeadsetOff,	&StartCall);
    InitStateNode (STATE_Ringing,			SMEV_CallSetup,					STATE_Ringing,			&RingingCallSetup);
    /*-------------------------------------------------------------------------------------------------*/

    /*---------------------------------- STATE: InCallHeadsetOn   --------------------------------------*/
    InitStateNode (STATE_InCallHeadsetOn,	SMEV_CallSetup,					STATE_InCallHeadsetOn,	NULLTRANS);
    InitStateNode (STATE_InCallHeadsetOn,	SMEV_Failure,					STATE_HfpConnected,		&EndCallVoiceFailure);
    InitStateNode (STATE_InCallHeadsetOn,	SMEV_EndCall,					STATE_HfpConnected,		&EndCall);
    InitStateNode (STATE_InCallHeadsetOn,	SMEV_HeadsetOff,				STATE_InCallHeadsetOff,	&StopConversation);
    InitStateNode (STATE_InCallHeadsetOn,	SMEV_AbonentInfo,				STATE_InCallHeadsetOn,	&AbonentInfo);
    /*--------------------------------------------------------------------------------------------------*/

    /*---------------------------------- STATE: InCallHeadsetOff  --------------------------------------*/
    InitStateNode (STATE_InCallHeadsetOff,	SMEV_CallSetup,					STATE_InCallHeadsetOff,	NULLTRANS);
    InitStateNode (STATE_InCallHeadsetOff,	SMEV_EndCall,					STATE_HfpConnected,		&EndCall);
    InitStateNode (STATE_InCallHeadsetOff,	SMEV_HeadsetOn,					STATE_InCallHeadsetOn,	&StartConversation);
    InitStateNode (STATE_InCallHeadsetOff,	SMEV_AbonentInfo,				STATE_InCallHeadsetOff,	&AbonentInfo);
    /*--------------------------------------------------------------------------------------------------*/

	UserCallback.Construct (cb);
    HfpSmObj.Construct();
}


void HfpSm::End ()
{
    HfpSmObj.Destruct();
}


	
/********************************************************************************\
								SM TRANSITIONS
\********************************************************************************/

bool HfpSm::SelectDevice (SMEVENT* ev, int param)
{
	uint64 addr = ev->Param.BthAddr;
	InHandDev * dev = InHand::FindDevice(addr);
	if (!dev) {
		LogMsg ("Failure to select device: %llX", addr);
		if (State == STATE_Idle)
			PutEvent_Failure();	// In order to jump back then to STATE_Idle
		goto end_func;
	}
	CurDevice = dev;
	UserCallback.DevicePresent (CurDevice);
	PutEvent_ConnectStart(addr);

	end_func:
    return true;
}


bool HfpSm::ForgetDevice (SMEVENT* ev, int param)
{
	CurDevice = 0;
	UserCallback.DeviceForgot();
    return true;
}


bool HfpSm::Connect (SMEVENT* ev, int param)
{
	InHand::BeginConnect(CurDevice->Address);
    return true;
}


bool HfpSm::Connected (SMEVENT* ev, int param)
{
	HfpSm::PutEvent_HfpConnectStart();
    return true;
}


bool HfpSm::HfpConnect (SMEVENT* ev, int param)
{
	InHand::BeginHfpConnect();
    return true;
}


bool HfpSm::HfpConnected (SMEVENT* ev, int param)
{
	UserCallback.HfpConnected();
    return true;
}


bool HfpSm::ConnectFailure (SMEVENT* ev, int param)
{
	UserCallback.ConnectFailure();
    return true;
}


bool HfpSm::ServiceConnectFailure (SMEVENT* ev, int param)
{
	UserCallback.ServiceConnectFailure();
    return true;
}


bool HfpSm::CallFailure (SMEVENT* ev, int param)
{
	UserCallback.CallFailure();
    return true;
}


bool HfpSm::IncorrectState4Call (SMEVENT* ev, int param)
{
	LogMsg ("IncorrectState4Call !!");
	UserCallback.OutgoingIncorrectState();
    return true;
}


bool HfpSm::OutgoingCall (SMEVENT* ev, int param)
{
	LogMsg ("Initiating call to: %s", ev->Param.CallNumber->Number);
	InHand::StartCall(ev->Param.CallNumber->Number);
	HeadsetOn = ev->Param.HeadsetOn;
	delete ev->Param.CallNumber;
	UserCallback.Calling();
    return true;
}


bool HfpSm::StartCall (SMEVENT* ev, int param)
{
	if (ev->Ev == SMEV_Answer) {
		HeadsetOn = ev->Param.HeadsetOn;
		InHand::Answer();
	}

	if (HeadsetOn)
		PutEvent_HeadsetOn();
	else
		UserCallback.InCallHeadsetOff();
    return true;
}


bool HfpSm::EndCall (SMEVENT* ev, int param)
{
	bool res;
	LogMsg ("Ending call...");
	InHand::EndCall();
	res = (HeadsetOn) ? StopConversation (ev, param) : true;
	UserCallback.CallEnded();
	return res;
}


bool HfpSm::EndCallVoiceFailure (SMEVENT* ev, int param)
{
	LogMsg ("Ending call because of voice channel problem");
    bool res = EndCall (ev, param);
	if (ev->Param.ReportFailure)
		UserCallback.CallEndedVoiceFailure();
	// else it may be normal case of closing channel, so do not report
	return res;
}


bool HfpSm::StartConversation (SMEVENT* ev, int param)
{
	LogMsg ("Starting conversation...");
	try
	{
		ScoAppObj->OpenSco (CurDevice->Address);
		ScoAppObj->VoiceStart();
		HeadsetOn = true;
		UserCallback.InCallHeadsetOn();
	}
	catch (int err)
	{
		LogMsg("EXCEPTION %d", err);
		HfpSm::PutEvent_Failure2Report();
	}
    return true;
}


bool HfpSm::StopConversation (SMEVENT* ev, int param)
{
	LogMsg ("Stopping conversation...");
	try
	{
		ScoAppObj->CloseSco();
		HeadsetOn = false;
		if (ev->Ev == SMEV_HeadsetOff)
			UserCallback.InCallHeadsetOff();
	}
	catch (int err)
	{
		LogMsg("EXCEPTION %d", err);
		HfpSm::PutEvent_Failure();
	}
    return true;
}


bool HfpSm::AbonentInfo (SMEVENT* ev, int param)
{
	if (State != STATE_HfpConnected)
		UserCallback.AbonentInfo(ev->Param.Abonent->Number);
	// When the current state is STATE_HfpConnected, this event occurred after some failure, to ignore it
	delete ev->Param.Abonent;
    return true;
}


bool HfpSm::Ringing (SMEVENT* ev, int param)
{
	UserCallback.Ring ();
    return true;
}


bool HfpSm::RingingCallSetup (SMEVENT* ev, int param)
{
	if (ev->Param.CallSetupStage == SMEV_CallSetup_None)
		PutEvent_EndCall();
    return true;
}

