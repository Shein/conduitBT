/********************************************************************************\
 Filename    :  HfpSm.cpp
 Purpose     :  Main HFP State Machine
\********************************************************************************/

#include "def.h"
#include "deblog.h"
#include "HfpSm.h"
#include "smBody.h"


IMPL_STATES (HfpSm, STATE_LIST_HFPSM)

HfpSm				HfpSmObj;
HfpSmCb  			HfpSm::UserCallback;
HfpSmInitReturn*	HfpSm::InitEvent;


//static
void HfpSm::ScoConnectCallback()
{
	SMEVENT Event = {SM_HFP, SMEV_SwitchVoice};
	Event.Param.PcSound = true;
	SmBase::PutEvent (&Event, SMQ_HIGH);
}


//static
void HfpSm::ScoDisconnectCallback()
{
	SMEVENT Event = {SM_HFP, SMEV_SwitchVoice};
	Event.Param.PcSound = false;
	SmBase::PutEvent (&Event, SMQ_HIGH);
}


//static
void HfpSm::ScoCritErrorCallback()
{
	::LogMsg ("SCO critical error...");
	SMEVENT Event = {SM_HFP, SMEV_SwitchVoice};
	Event.Param.PcSound = false;
	Event.Param.ReportError = DialAppError_OpenScoFailure;
	SmBase::PutEvent (&Event, SMQ_HIGH);
}


void HfpSm::Construct()
{
	MyTimer.Construct ();
	SMT<HfpSm>::Construct (SM_HFP);
	ScoAppObj = new ScoApp (ScoConnectCallback, ScoDisconnectCallback, ScoCritErrorCallback);
}


void HfpSm::Destruct()
{
	delete ScoAppObj;
	MyTimer.Destruct();
}


void HfpSm::Init (DialAppCb cb, HfpSmInitReturn* initevent)
{
	/*---------------------------------- STATE: Init ---------------------------------------------------*/
	InitStateNode (STATE_Init,				SMEV_Error,						&ChoiceProcessInit,		2);
		InitChoice (0, STATE_Init,			SMEV_Error,						STATE_Init,				NULLTRANS);
		InitChoice (1, STATE_Init,			SMEV_Error,						STATE_Idle,				NULLTRANS);
	/*-------------------------------------------------------------------------------------------------*/

	/*---------------------------------- STATE: Idle ---------------------------------------------------*/
	InitStateNode (STATE_Idle,				SMEV_SelectDevice,				STATE_Disconnected,		&SelectDevice);
	InitStateNode (STATE_Idle,				SMEV_StartOutgoingCall,			STATE_Idle,				&IncorrectState4Call);
	InitStateNode (STATE_Idle,				SMEV_SwitchHeadset,				STATE_Idle,				&SwitchVoiceOnOff);
	/*-------------------------------------------------------------------------------------------------*/

	/*---------------------------------- STATE: Disconnected ------------------------------------------*/
	InitStateNode (STATE_Disconnected,		SMEV_ForgetDevice,				STATE_Idle,				&ForgetDevice);
	InitStateNode (STATE_Disconnected,		SMEV_SelectDevice,				STATE_Disconnected,		&SelectDevice);
	InitStateNode (STATE_Disconnected,		SMEV_Disconnect,				STATE_Disconnected,		NULLTRANS);
	InitStateNode (STATE_Disconnected,		SMEV_ConnectStart,				STATE_Connecting,		&Connect);
	InitStateNode (STATE_Disconnected,		SMEV_Timeout,					STATE_Connecting,		&Connect);
	InitStateNode (STATE_Disconnected,		SMEV_StartOutgoingCall,			STATE_Disconnected,		&IncorrectState4Call);
	InitStateNode (STATE_Disconnected,		SMEV_SwitchHeadset,				STATE_Disconnected,		&SwitchVoiceOnOff);
	/*-------------------------------------------------------------------------------------------------*/

	/*---------------------------------- STATE: Connecting   ------------------------------------------*/
	InitStateNode (STATE_Connecting,		SMEV_ForgetDevice,				STATE_Idle,				&ForgetDevice);
	InitStateNode (STATE_Connecting,		SMEV_SelectDevice,				STATE_Disconnected,		&SelectDevice);
	InitStateNode (STATE_Connecting,		SMEV_Disconnect,				STATE_Disconnected,		&Disconnect);
	InitStateNode (STATE_Connecting,		SMEV_Error,						STATE_Disconnected,		&ConnectFailure);
	InitStateNode (STATE_Connecting,		SMEV_Connected,					STATE_Connected,		&Connected);
	InitStateNode (STATE_Connecting,		SMEV_StartOutgoingCall,			STATE_Connecting,		&IncorrectState4Call);
	InitStateNode (STATE_Connecting,		SMEV_SwitchHeadset,				STATE_Connecting,		&SwitchVoiceOnOff);
	/*-------------------------------------------------------------------------------------------------*/

	/*---------------------------------- STATE: Connected   -------------------------------------------*/
	InitStateNode (STATE_Connected,			SMEV_ForgetDevice,				STATE_Idle,				&ForgetDevice);
	InitStateNode (STATE_Connected,			SMEV_SelectDevice,				STATE_Disconnected,		&SelectDevice);
	InitStateNode (STATE_Connected,			SMEV_Disconnect,				STATE_Disconnected,		&Disconnect);
	InitStateNode (STATE_Connected,			SMEV_HfpConnectStart,			STATE_HfpConnecting,	&HfpConnect);
	InitStateNode (STATE_Connected,			SMEV_StartOutgoingCall,			STATE_Connected,		&IncorrectState4Call);
	InitStateNode (STATE_Connected,			SMEV_SwitchHeadset,				STATE_Connected,		&SwitchVoiceOnOff);
	/*-------------------------------------------------------------------------------------------------*/

	// HfpConnecting is interesting state: when the application starts to run it may receive SMEV_SwitchVoice and other events 
	/*---------------------------------- STATE: HfpConnecting   ---------------------------------------*/
	InitStateNode (STATE_HfpConnecting,		SMEV_ForgetDevice,				STATE_Idle,				&ForgetDevice);
	InitStateNode (STATE_HfpConnecting,		SMEV_SelectDevice,				STATE_Disconnected,		&SelectDevice);
	InitStateNode (STATE_HfpConnecting,		SMEV_Disconnect,				STATE_Disconnected,		&Disconnect);
	InitStateNode (STATE_HfpConnecting,		SMEV_Error,						STATE_Disconnected,		&ServiceConnectFailure);
	InitStateNode (STATE_HfpConnecting,		SMEV_StartOutgoingCall,			STATE_HfpConnecting,	&IncorrectState4Call);
	InitStateNode (STATE_HfpConnecting,		SMEV_SwitchHeadset,				STATE_HfpConnecting,	&SwitchVoiceOnOff);
	InitStateNode (STATE_HfpConnecting,		SMEV_SwitchVoice,				STATE_HfpConnecting,	&SwitchedVoiceOnOff);
	InitStateNode (STATE_HfpConnecting,		SMEV_Timeout,					STATE_Disconnected,		&ServiceConnectFailure);
	InitStateNode (STATE_HfpConnecting,		SMEV_AtResponse,				&IsHfpConnectLastCmd,	6);
		InitChoice (0, STATE_HfpConnecting,	SMEV_AtResponse,				STATE_HfpConnecting,	&AtProcessing);
		InitChoice (1, STATE_HfpConnecting,	SMEV_AtResponse,				STATE_Disconnected,		&ServiceConnectFailure);
		InitChoice (2, STATE_HfpConnecting,	SMEV_AtResponse,				STATE_HfpConnected,		&HfpConnected);
		InitChoice (3, STATE_HfpConnecting,	SMEV_AtResponse,				STATE_Calling,			&HfpConnected_CallFromPhone);
		InitChoice (4, STATE_HfpConnecting,	SMEV_AtResponse,				STATE_Ringing,			&HfpConnected_Ringing);
		InitChoice (5, STATE_HfpConnecting,	SMEV_AtResponse,				STATE_InCall,			&HfpConnected_StartCall);
	/*------------------------------------------------------------------------------------------------*/

	/*---------------------------------- STATE: HfpConnected   ----------------------------------------*/
	InitStateNode (STATE_HfpConnected,		SMEV_ForgetDevice,				STATE_Idle,				&ForgetDevice);
	InitStateNode (STATE_HfpConnected,		SMEV_SelectDevice,				STATE_Disconnected,		&SelectDevice);
	InitStateNode (STATE_HfpConnected,		SMEV_Disconnect,				STATE_Disconnected,		&Disconnect);
	InitStateNode (STATE_HfpConnected,		SMEV_CallEnded,					STATE_HfpConnected,		NULLTRANS);
	InitStateNode (STATE_HfpConnected,		SMEV_SwitchHeadset,				STATE_HfpConnected,		&SwitchVoiceOnOff);
	InitStateNode (STATE_HfpConnected,		SMEV_StartOutgoingCall,			STATE_Calling,			&StartOutgoingCall);
	InitStateNode (STATE_HfpConnected,		SMEV_AtResponse,				&ToRingingOrCalling,	3);
		InitChoice (0, STATE_HfpConnected,	SMEV_AtResponse,				STATE_HfpConnected,		&AtProcessing);
		InitChoice (1, STATE_HfpConnected,	SMEV_AtResponse,				STATE_Ringing,			&Ringing);
		InitChoice (2, STATE_HfpConnected,	SMEV_AtResponse,				STATE_Calling,			&CallFromPhone);
	/*-------------------------------------------------------------------------------------------------*/

	/*---------------------------------- STATE: Calling   ---------------------------------------------*/
	InitStateNode (STATE_Calling,			SMEV_Disconnect,				STATE_Disconnected,		&Disconnect);
	InitStateNode (STATE_Calling,			SMEV_Error,						STATE_HfpConnected,		&EndCall);
	InitStateNode (STATE_Calling,			SMEV_CallEnd,					STATE_HfpConnected,		&EndCall);
	InitStateNode (STATE_Calling,			SMEV_CallEnded,					STATE_HfpConnected,		&EndCall);
	InitStateNode (STATE_Calling,			SMEV_CallStart,					STATE_InCall,			&StartCall);
	InitStateNode (STATE_Calling,			SMEV_SwitchHeadset,				STATE_Calling,			&SwitchVoiceOnOff);
	InitStateNode (STATE_Calling,			SMEV_SwitchVoice,				STATE_Calling,			&SwitchedVoiceOnOff);
	InitStateNode (STATE_Calling,			SMEV_AtResponse,				&ChoiceCallSetup,		3);
		InitChoice (0, STATE_Calling,		SMEV_AtResponse,				STATE_HfpConnected,		&EndCall);
		InitChoice (1, STATE_Calling,		SMEV_AtResponse,				STATE_Calling,			&OutgoingCall);
		InitChoice (2, STATE_Calling,		SMEV_AtResponse,				STATE_Calling,			&AtProcessing);
	/*-------------------------------------------------------------------------------------------------*/

	/*---------------------------------- STATE: Ringing   ---------------------------------------------*/
	InitStateNode (STATE_Ringing,			SMEV_Disconnect,				STATE_Disconnected,		&Disconnect);
	InitStateNode (STATE_Ringing,			SMEV_Error,						STATE_HfpConnected,		&EndCall);
	InitStateNode (STATE_Ringing,			SMEV_CallEnd,					STATE_HfpConnected,		&EndCall);
	InitStateNode (STATE_Ringing,			SMEV_CallEnded,					STATE_HfpConnected,		&EndCall);
	InitStateNode (STATE_Ringing,			SMEV_Answer,					STATE_Ringing,			&Answer);
	InitStateNode (STATE_Ringing,			SMEV_CallStart,					STATE_InCall,			&StartCall);
	InitStateNode (STATE_Ringing,			SMEV_SwitchHeadset,				STATE_Ringing,			&SwitchVoiceOnOff);
	InitStateNode (STATE_Ringing,			SMEV_SwitchVoice,				STATE_Ringing,			&SwitchedVoiceOnOff);
	InitStateNode (STATE_Ringing,			SMEV_AtResponse,				&ChoiceFromRinging,		2);
		InitChoice (0, STATE_Ringing,		SMEV_AtResponse,				STATE_HfpConnected,		&EndCall);
		InitChoice (1, STATE_Ringing,		SMEV_AtResponse,				STATE_Ringing,			&AtProcessing);
	/*-------------------------------------------------------------------------------------------------*/

	/*---------------------------------- STATE: InCall  ------------------------------------------------*/
	InitStateNode (STATE_InCall,			SMEV_Disconnect,				STATE_Disconnected,		&Disconnect);
	InitStateNode (STATE_InCall,			SMEV_Error,						STATE_InCall,			&StopVoice);
	InitStateNode (STATE_InCall,			SMEV_AtResponse,				STATE_InCall,			&AtProcessing);
	InitStateNode (STATE_InCall,			SMEV_SendDtmf,					STATE_InCall,			&SendDtmf);
	InitStateNode (STATE_InCall,			SMEV_Answer,					STATE_InCall,			&Answer2Waiting);
	InitStateNode (STATE_InCall,			SMEV_PutOnHold,					STATE_InCall,			&PutOnHold);
	InitStateNode (STATE_InCall,			SMEV_CallWaiting,				STATE_InCall,			&IncomingWaitingCall);
	InitStateNode (STATE_InCall,			SMEV_Timeout,					STATE_InCall,			&SendWaitingCallStop);
	InitStateNode (STATE_InCall,			SMEV_SwitchHeadset,				STATE_InCall,			&SwitchVoiceOnOff);
	InitStateNode (STATE_InCall,			SMEV_SwitchVoice,				&ChoiceIncomingVoice,	2);
		InitChoice (0, STATE_InCall,		SMEV_SwitchVoice,				STATE_InCall,			&RejectVoice);
		InitChoice (1, STATE_InCall,		SMEV_SwitchVoice,				STATE_InCall,			&SwitchedVoiceOnOff);
	InitStateNode (STATE_InCall,			SMEV_CallHeld,					STATE_InCall,			&CallHeld);
	InitStateNode (STATE_InCall,			SMEV_CallEnd,					STATE_InCall,			&StartCallEnding);
	InitStateNode (STATE_InCall,			SMEV_CallEnded,					STATE_HfpConnected,		&FinalizeCallEnding);
	/*--------------------------------------------------------------------------------------------------*/

	HfpSm::InitEvent = initevent;
	UserCallback.Construct (cb);
	HfpSmObj.Construct();
}


void HfpSm::End ()
{
	HfpSmObj.Destruct();
}


/********************************************************************************\
								SM Help functions
\********************************************************************************/


bool HfpSm::WasPcsoundPrefChanged (SMEVENT* ev)
{
	if (PublicParams.PcSoundPref != ev->Param.PcSound) {
		PublicParams.PcSoundPref = ev->Param.PcSound;
		LogMsg ("PcSoundPref = %d", PublicParams.PcSoundPref);
		return true;
	}
	return false;
}


void HfpSm::StartVoiceHlp (bool waveonly)
{
	LogMsg ("Starting voice...");
	try
	{
		ScoAppObj->OpenSco(waveonly);
		ScoAppObj->VoiceStart();
		PublicParams.PcSound = true;
	}
	catch (int err)
	{
		LogMsg("EXCEPTION %d", err);
		HfpSm::PutEvent_Failure(DialAppError_OpenScoFailure);	// if we have an error when starting voice, we should to notice
	}
}


void HfpSm::StopVoiceHlp (bool waveonly)
{
	LogMsg ("Stopping voice...");
	try
	{
		ScoAppObj->CloseSco(waveonly);
	}
	catch (int err)
	{
		LogMsg("EXCEPTION %d", err);
		HfpSm::PutEvent_Failure(DialAppError_CoseScoFailure);	// if we have an error when stopping voice, we should not to notice, it may be the normal case
	}
	PublicParams.PcSound = false;
}


void HfpSm::ClearAllCallInfo ()
{
	if (CallInfoCurrent)
		delete CallInfoCurrent;
	if (CallInfoWaiting)
		delete CallInfoWaiting;
	if (CallInfoHeld)
		delete CallInfoHeld;
	if (CallInfoWaiting)
		delete CallInfoWaiting;

	CallInfoCurrent = CallInfoHeld = CallInfoWaiting = 0;
	PublicParams.ClearAbonentCurrent();
	PublicParams.ClearAbonentWaiting();
	PublicParams.ClearAbonentHeld();
	LogMsg("Set CallInfoCurrent=CallInfoHeld=CallInfoWaiting=0");
}


void HfpSm::SetCallInfo4CurrentCall (CallInfo<char> *info)
{
	CallInfo<char> *prev = (CallInfoCurrent) ? CallInfoCurrent : 0;

	if (info) {
		CallInfoCurrent = info;
		CallInfoCurrent->Parse2NumberName ();
		PublicParams.SetAbonentCurrent(CallInfoCurrent);
	}
	else {
		CallInfoCurrent = 0;
		PublicParams.ClearAbonentCurrent();
	}

	if (!CallInfoCurrent || !CallInfoCurrent->Compare(prev))
		UserCallback.CallCurrentInfo();

	if (prev)
		delete prev;
}


void HfpSm::SetCallInfo4WaitingCall (CallInfo<char> *info)
{
	if (CallInfoWaiting)
		delete CallInfoWaiting;

	if (info) {
		CallInfoWaiting = info;
		CallInfoWaiting->Parse2NumberName ();
		PublicParams.SetAbonentWaiting(CallInfoWaiting);
	}
	else {
		CallInfoWaiting = 0;
		PublicParams.ClearAbonentWaiting();
	}
	UserCallback.CallWaitingInfo();
}


void HfpSm::SetCallInfo4HeldCall (SMEV_ATRESPONSE heldstatus)
{
	uint32 addflag = 0;

	switch (heldstatus)
	{
		case SMEV_AtResponse_CallHeld_None:			// No held call: clear the kept 
		case SMEV_AtResponse_CallHeld_HeldOnly:		// Current call=>held: clear the kept and set held = current 
			if (CallInfoHeld) {
				LogMsg("Deleting CallInfoHeld");
				delete CallInfoHeld;
				CallInfoHeld = 0;
				PublicParams.AbonentHeld = 0;
			}
			if (heldstatus == SMEV_AtResponse_CallHeld_None)
				break;
			CallInfoHeld = CallInfoCurrent;
			CallInfoCurrent = 0;
			PublicParams.SetAbonentHeld (CallInfoHeld);
			PublicParams.ClearAbonentCurrent();
			addflag = DIALAPP_FLAG_ABONENT_CURRENT;
			LogMsg("Set CallInfoHeld=CallInfoCurrent(%X), CallInfoCurrent=0", CallInfoCurrent);
			break;

		case SMEV_AtResponse_CallHeld_HeldAndActive:	
			// Switch the held call with the current or with the waiting
			// For now we will use CallInfoWaiting to understand were we are
			if (CallInfoWaiting) {
				// Probably we are switching held and waiting
				if (CallInfoHeld)
					delete CallInfoHeld;
				CallInfoHeld = CallInfoWaiting;
				CallInfoWaiting = 0;
				PublicParams.SetAbonentHeld	(CallInfoHeld);
				PublicParams.ClearAbonentWaiting ();
				addflag = DIALAPP_FLAG_ABONENT_WAITING;
				LogMsg("Set CallInfoHeld=CallInfoWaiting(%X), CallInfoWaiting=0", CallInfoWaiting);
			}
			else 
			{
				// Probably we are switching held and current 
				CallInfo<char> *c = CallInfoHeld;
				CallInfoHeld = CallInfoCurrent;
				CallInfoCurrent = c;
				PublicParams.SetAbonentCurrent (CallInfoCurrent);
				PublicParams.SetAbonentHeld (CallInfoHeld);
				addflag = DIALAPP_FLAG_ABONENT_CURRENT;
				LogMsg("Switch CallInfoHeld<=>CallInfoCurrent(%X<=>%X)", CallInfoCurrent, CallInfoHeld);
			}
			break;

		default:
			LogMsg("Incorrect Held Status");
			return;
	}

	UserCallback.CallHeldInfo(addflag);
}


bool HfpSm::IsCurStateSupportingVoiceSwitch()
{
	if (PublicParams.PcSound && !PublicParams.PcSoundPref) {
		// Currently PS Sound is active but a user prefers to switch it off.
		// It is always permitted to stop sending voice to PS Sound devices, not related to the cur SM state:
		// it may be switched on erroneously, e.g. as result of some bug, but we should have possibility to switch it off.
		return true;
	}

	return (State > STATE_HfpConnected);
}

	
/********************************************************************************\
								SM TRANSITIONS
\********************************************************************************/

bool HfpSm::SwitchVoiceOnOff (SMEVENT* ev, int param)
{
	if (!WasPcsoundPrefChanged(ev)) {
		LogMsg ("WARNING: Acquiring PcSoundPref state is already set, do nothing");
		return true;
	}

	// Here the PcSoundPref is switched 

	if (IsCurStateSupportingVoiceSwitch()) 
	{
		if (PublicParams.PcSoundPref)	// StartVoiceHlp/StopVoiceHlp set PublicParams.PcSound to the correspondent state
			StartVoiceHlp(false);
		else
			StopVoiceHlp(false);
		// Inform about PcSound and PcSoundPref both flags change
		UserCallback.PcSoundOnOff (DIALAPP_FLAG_PCSOUND);
	}
	else 
	{
		ScoAppObj->SetIncomingReadiness (PublicParams.PcSoundPref);
		// The voice channel is closed, inform about PcSoundPref flag change only
		UserCallback.PcSoundOnOff();
	}
	return true;
}


bool HfpSm::SwitchedVoiceOnOff (SMEVENT* ev, int param)
{
	if (ev->Param.PcSound == PublicParams.PcSound) {
		if (ev->Param.ReportError) {
			// Probably the opening SCO was failed
			UserCallback.NotifyFailure (ev->Param.ReportError);
		}
		else {
			// Event bringing PcSound state that is already set
			LogMsg ("WARNING: Acquiring PcSound state is already set, do nothing");
		}
		return true;
	}

	if (PublicParams.PcSoundPref = ev->Param.PcSound)  { 
		// StartVoiceHlp/StopVoiceHlp set PublicParams.PcSound to the correspondent state
		// The voice channel is currently active! Start Waves only
		StartVoiceHlp(true);
	}
	else {
		StopVoiceHlp(true);
	}
	UserCallback.PcSoundOnOff (DIALAPP_FLAG_PCSOUND);
	return true;
}


bool HfpSm::SelectDevice (SMEVENT* ev, int param)
{
	MyTimer.Stop();	

	uint64 addr = ev->Param.BthAddr;
	DialAppBthDev * dev = InHand::FindDevice(addr);
	if (!dev) {
		LogMsg ("Failure to select device: %llX", addr);
		if (State == STATE_Idle)
			PutEvent_ForgetDevice();	// In order to jump back then to STATE_Idle (may be changed to Choice)
		UserCallback.DeviceUnknown();
		goto end_func;
	}

	if (State > STATE_Disconnected)
		InHand::Disconnect();

	LogMsg ("Selected device: %llX, %s", addr, dev->Name);
	PublicParams.CurDevice = dev;
	UserCallback.DevicePresent();
	MyTimer.Start(TIMEOUT_CONNECTION_POLLING,true);
	PutEvent_ConnectStart(addr);

	end_func:
	if (State >= STATE_HfpConnected)
		ScoAppObj->StopServer();
	return true;
}


bool HfpSm::ForgetDevice (SMEVENT* ev, int param)
{
	MyTimer.Stop();	
	void* prevdev = PublicParams.CurDevice;
	PublicParams.CurDevice = 0;
	// Disconnect and Report about forgot device in the case it was present before, 
	// otherwise it may be the case of failure in HfpSm::SelectDevice in Idle state
	if (prevdev) {
		if (State >= STATE_HfpConnected)
			ScoAppObj->StopServer();
		if (State > STATE_Disconnected)
			InHand::Disconnect();
		UserCallback.DeviceForgot();
	}
	return true;
}


bool HfpSm::Disconnect (SMEVENT* ev, int param)
{
	if (State >= STATE_HfpConnecting)
		ScoAppObj->StopServer();
	if (State > STATE_Disconnected)
		InHand::Disconnect();
	MyTimer.Start (TIMEOUT_CONNECTION_POLLING, true);
	if (ev->Param.ReportError)
		UserCallback.NotifyFailure (ev->Param.ReportError);
	else
		UserCallback.DevicePresent (0);
	ClearAllCallInfo();

	if (State == STATE_InCall) {
		// After leaving InCall state need return PcSoundPref to the SCO driver
		ScoAppObj->SetIncomingReadiness (PublicParams.PcSoundPref);
	}
	return true;
}


bool HfpSm::Connect (SMEVENT* ev, int param)
{
	MyTimer.Start (TIMEOUT_CONNECTION_POLLING, true);
	InHand::BeginConnect (PublicParams.CurDevice->Address);
	return true;
}


bool HfpSm::Connected (SMEVENT* ev, int param)
{
	// In the case of timeout the timer is stopped because of single timer event 
	if (ev->Ev != SMEV_Timeout)
		MyTimer.Stop();	
	HfpSm::PutEvent_HfpConnectStart();
	return true;
}


bool HfpSm::HfpConnect (SMEVENT* ev, int param)
{
	// In the case HFP negotiation will not be completed in the given time,
	// we assume that it's ok and will jump to the next state
	HfpIndicatorsState = -1;
	InHand::ClearIndicatorsNumbers();
	try
	{
		ScoAppObj->StartServer (PublicParams.CurDevice->Address, PublicParams.PcSoundPref);
		MyTimer.Start(TIMEOUT_HFP_NEGOTIATION,true);
		InHand::BeginHfpConnect();
	}
	catch (int err)
	{
		LogMsg("EXCEPTION %d", err);
		HfpSm::PutEvent_Disconnect(DialAppError_ServiceConnectFailure);
	}
	return true;
}


bool HfpSm::HfpConnected (SMEVENT* ev, int param)
{
	MyTimer.Stop();

	// After STATE_HfpConnecting we can also jump to call states, they should have own callbacks
	if (State_next == STATE_HfpConnected)
		UserCallback.HfpConnected();

	if (State_next == STATE_HfpConnected && PublicParams.PcSound) {
		// The audio SCO was open during HFP negotiation, probably erroneously. We are closing it... 
		StopVoiceHlp(false);
		UserCallback.PcSoundOnOff (DIALAPP_FLAG_PCSOUND);
	}

	return true;
}


bool HfpSm::ConnectFailure (SMEVENT* ev, int param)
{
	MyTimer.Start(TIMEOUT_CONNECTION_POLLING,true);
	UserCallback.NotifyFailure (DialAppError_ConnectFailure);
	return true;
}


bool HfpSm::ServiceConnectFailure (SMEVENT* ev, int param)
{
	InHand::Disconnect ();
	// Here must return to Disconnected state 
	MyTimer.Start(TIMEOUT_CONNECTION_POLLING,true);
	UserCallback.NotifyFailure (DialAppError_ServiceConnectFailure);
	return true;
}


bool HfpSm::EndCall (SMEVENT* ev, int param)
{
	LogMsg ("Ending call...");
	InHand::EndCall();
	if (PublicParams.PcSound)
		StopVoiceHlp(false);
	if (ev->Param.ReportError)
		UserCallback.NotifyFailure (ev->Param.ReportError);
	else if (ev->Ev == SMEV_Error)
		UserCallback.NotifyFailure (DialAppError_CallFailure);
	else
		UserCallback.CallEnded();
	ClearAllCallInfo();
	return true;
}


bool HfpSm::StartCallEnding (SMEVENT* ev, int param)
{
	LogMsg ("Ending current call...");
	InHand::EndCall();
	return true;
}


bool HfpSm::FinalizeCallEnding (SMEVENT* ev, int param)
{
	LogMsg ("Current call ended...");
	if (PublicParams.PcSound)
		StopVoiceHlp(false);
	if (ev->Ev == SMEV_Error)
		UserCallback.NotifyFailure (DialAppError_CallFailure);
	else
		UserCallback.CallEnded();
	ClearAllCallInfo();
	ScoAppObj->SetIncomingReadiness (PublicParams.PcSoundPref);
	return true;
}


bool HfpSm::IncorrectState4Call (SMEVENT* ev, int param)
{
	LogMsg ("IncorrectState4Call !!");
	UserCallback.NotifyFailure (DialAppError_IncorrectState4Call);
	return true;
}


bool HfpSm::StartOutgoingCall (SMEVENT* ev, int param)
{
	LogMsg ("Initiating call to: %s", ev->Param.CallNumber->Info);
	InHand::StartCall(ev->Param.CallNumber->Info);
	delete ev->Param.CallNumber;
	UserCallback.Calling();
	return true;
}


bool HfpSm::OutgoingCall (SMEVENT* ev, int param)
{
	LogMsg ("PublicParams: PcSoundPref = %d, PcSound = %d", PublicParams.PcSoundPref, PublicParams.PcSound);
	InHand::ListCurrentCalls();
	return true;
}


bool HfpSm::CallFromPhone (SMEVENT* ev, int param)
{
	LogMsg ("Initiating call from phone");
	InHand::ListCurrentCalls();
	UserCallback.Calling();
	return true;
}


bool HfpSm::Answer (SMEVENT* ev, int param)
{
	InHand::Answer();
	return true;
}


bool HfpSm::StartCall (SMEVENT* ev, int param)
{
	UserCallback.InCall();
	// When we are InCall - always to receive Connect requests
	ScoAppObj->SetIncomingReadiness (true);
	// In the case we don't want to switch PC sound on automatically - save the current timestamp
	IncallStartTime = (PublicParams.PcSoundPref)  ?  0 : Timer::GetCurMilli();
	return true;
}


bool HfpSm::StopVoice (SMEVENT* ev, int param)
{
	// For now this is the response to Failure (probably SCO read/write) in InCall state
	if (PublicParams.PcSound) {
		// the PublicParams.PcSoundPref is unchanged here
		StopVoiceHlp(false);
		UserCallback.PcSoundOff();
	}
	return true;
}


bool HfpSm::RejectVoice (SMEVENT* ev, int param)
{
	// This func is for InCall state only
	LogMsg("Reject Incoming voice because PcSoundPref = 0");
	ScoAppObj->CloseScoLowLevel();
	return true;
}


bool HfpSm::Ringing (SMEVENT* ev, int param)
{
	InHand::ListCurrentCalls();
	UserCallback.Ring();
	return true;
}


bool HfpSm::IncomingWaitingCall (SMEVENT* ev, int param)
{
	if (ev->Param.AtResponse == SMEV_AtResponse_CallWaiting_Ringing) {
		LogMsg("IncomingWaitingCall: Ringing");
		SetCallInfo4WaitingCall (ev->Param.InfoCh);
	}
	else {
		// SMEV_AtResponse_CallWaiting_Stopped
		LogMsg("IncomingWaitingCall: Stopped");
		if (CallInfoWaiting)
			SetCallInfo4WaitingCall(0);
	}

	return true;
}


bool HfpSm::SendWaitingCallStop (SMEVENT* ev, int param)
{
	PutEvent_CallWaitingStopped();
	return true;
}


bool HfpSm::CallHeld (SMEVENT* ev, int param)
{
	MyTimer.Stop();	// Was set to TIMEOUT_WAITING_HOLD_SWITCH in AtProcessing when SMEV_AtResponse_CallSetup_None
	SetCallInfo4HeldCall (ev->Param.AtResponse);
	return true;
}


bool HfpSm::AtProcessing (SMEVENT* ev, int param)
{
	switch (ev->Param.AtResponse)
	{
		case SMEV_AtResponse_Ok:
		case SMEV_AtResponse_Error:
			// These events not used already
			break;

		case SMEV_AtResponse_ListCurrentCalls:
		case SMEV_AtResponse_CallingLineId:
			SetCallInfo4CurrentCall (ev->Param.InfoCh);
			break;

		case SMEV_AtResponse_CallSetup_None:
			if (State == STATE_InCall && CallInfoWaiting) {
				// We are in the InCall and if after receiving Waiting call it stopped to ring, so delete its information
				// For this purpose we use short timeout to wait the possible subsequent callheld event.
				MyTimer.Start(TIMEOUT_WAITING_HOLD_SWITCH,true);
			}
			break;
	}

	return true;
}


bool HfpSm::SendDtmf(SMEVENT* ev, int param)
{
	InHand::SendDtmf(&ev->Param.Dtmf);
	return true;
}


bool HfpSm::PutOnHold(SMEVENT* ev, int param)
{
	if (CallInfoHeld)
		LogMsg("About to switch Current/Held calls");
	else 
		LogMsg("About to put Current call on hold");
	InHand::PutOnHold();
	return true;
}


bool HfpSm::Answer2Waiting(SMEVENT* ev, int param)
{
	if (CallInfoWaiting) {
		LogMsg("Answering on Waiting incoming call");
		//InHand::PutOnHold(); 
		// this is moved before return as workaround for iPhone 
		// (it has not call waiting notifications, so CallInfoWaiting is always 0)
	}
	else {
		LogMsg("NO WAITING CALLS TO ANSWER");
	}
	InHand::PutOnHold();
	return true;
}


bool HfpSm::HfpConnected_CallFromPhone (SMEVENT* ev, int param)
{
	HfpConnected (ev, param);
	return CallFromPhone (ev, param);
}



bool HfpSm::HfpConnected_Ringing (SMEVENT* ev, int param)
{
	HfpConnected (ev, param);
	return Ringing (ev, param);
}


bool HfpSm::HfpConnected_StartCall (SMEVENT* ev, int param)
{
	HfpConnected (ev, param);
	return StartCall (ev, param);
}



/********************************************************************************\
								SM CHOICES
\********************************************************************************/

int HfpSm::ChoiceProcessInit (SMEVENT* ev)
{
	// When HfpSm::InitEvent is 0 it means the UserCallback.InitialCallback already reported an error;
	// in this case the application must break execution and do not continue to send events to the SM.
	// In this situation other errors/events are ignored
	int ret = 0;
	if (HfpSm::InitEvent)
	{
		int err = ev->Param.ReportError;
		switch (err)
		{
			case DialAppError_Ok:
				if (++InitEventsCnt < 2)  // 2 Ok reports: WaveIn & WaveOut
					return ret;	// Still stay in Init and wait next events
				ret = 1; // goto STATE_Idle
				break;
			default:
				ret = 0; // no matter were the SM is, but stay in STATE_Init
		}

		HfpSm::InitEvent->RetCode = err;
		HfpSm::InitEvent->SignalEvent.Signal();
		if (err == DialAppError_Ok) {
			// call user's callback reporting that the init finished successfully when no errors only, 
			// otherwise it's no needing in any callback - the application must stop.
			UserCallback.InitialCallback();
		}
		// HfpSm::InitEvent is one time use only
		HfpSm::InitEvent = 0;
	}
	return ret; 
}


int HfpSm::ToRingingOrCalling (SMEVENT* ev)
{
	switch (ev->Param.AtResponse)
	{
		case SMEV_AtResponse_CallSetup_Incoming:
			LogMsg("CallSetup_Incoming (PcSoundPref = %d)", PublicParams.PcSoundPref);
			return 1; // to STATE_Ringing
		case SMEV_AtResponse_CallSetup_Outgoing:
			LogMsg("CallSetup_Outgoing (PcSoundPref = %d)", PublicParams.PcSoundPref);
			return 2; // to STATE_Ringing
	}
	return 0; // call AtProcessing
}


int HfpSm::ChoiceFromRinging (SMEVENT* ev)
{
	switch (ev->Param.AtResponse)
	{
		case SMEV_AtResponse_CallSetup_None:
			LogMsg("CallSetup_None (PcSoundPref = %d)", PublicParams.PcSoundPref);
			return 0;	// back to STATE_HfpConnected
	}
	return 1; // stay in STATE_Ringing, call AtProcessing
}


int HfpSm::ChoiceCallSetup (SMEVENT* ev)
{
	switch (ev->Param.AtResponse)
	{
		case SMEV_AtResponse_Error:
		case SMEV_AtResponse_CallSetup_None:
			return 0;	// call failure or call terminated - back to STATE_HfpConnected
		case SMEV_AtResponse_CallSetup_Outgoing:
			LogMsg("CallSetup_Outgoing (PcSoundPref = %d)", PublicParams.PcSoundPref);
			return 1;	// OutgoingCall
	}
	return 2;	// stay in STATE_Calling, call AtProcessing
}


int HfpSm::ChoiceIncomingVoice (SMEVENT* ev)
{
	if (IncallStartTime  &&  (int(Timer::GetCurMilli() - IncallStartTime) < TIMEOUT_SCO_TREATED_AS_INCOMING))
		return 0; // Going to reject the incoming SCO

	// if to be very accurate, IncallStartTime 0 value shroud not be used here as a no-value flag, 
	// because of this Windows counter may be wrapped around, but it is unreal... KS
	IncallStartTime = 0;

	return 1; // Normal situation 
}


int HfpSm::IsHfpConnectLastCmd (SMEVENT* ev)
{
	ASSERT__ (State == STATE_HfpConnecting);

	if (ev->Param.AtResponse != SMEV_AtResponse_CurrentPhoneIndicators)
		return 0; // simply to call AtProcessing and to stay in STATE_HfpConnecting
	
	HfpIndicatorsState = ev->Param.IndicatorsState;
	if (HfpIndicatorsState < 0)
		return 1;	// go to STATE_Disconnected

	// Now jump to 2..5 (STATE_HfpConnected..STATE_InCall)
	return HfpIndicatorsState - STATE_HfpConnected + 2;
}
