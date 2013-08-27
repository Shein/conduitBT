/********************************************************************************\
 Filename    :  HfpSm.cpp
 Purpose     :  Main HFP State Machine
\********************************************************************************/

#include "def.h"
#include "deblog.h"
#include "str.h"
#include "HfpSm.h"
#include "smBody.h"


IMPL_STATES (HfpSm, STATE_LIST_HFPSM)

HfpSm		HfpSmObj;
HfpSmCb  	HfpSm::UserCallback;


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


void HfpSm::Construct()
{
	MyTimer.Construct ();
	SMT<HfpSm>::Construct (SM_HFP);
	ScoAppObj = new ScoApp (ScoConnectCallback, ScoDisconnectCallback);
}


void HfpSm::Destruct()
{
	delete ScoAppObj;
	MyTimer.Destruct();
}


void HfpSm::Init (DialAppCb cb)
{
	/*---------------------------------- STATE: Idle ---------------------------------------------------*/
	InitStateNode (STATE_Idle,				SMEV_SelectDevice,				STATE_Disconnected,		&SelectDevice);
	InitStateNode (STATE_Idle,				SMEV_StartOutgoingCall,			STATE_Idle,				&IncorrectState4Call);
	InitStateNode (STATE_Idle,				SMEV_SwitchHeadset,				STATE_Idle,				&SetPcsoundPrefFlag);
	/*-------------------------------------------------------------------------------------------------*/

	/*---------------------------------- STATE: Disconnected ------------------------------------------*/
	InitStateNode (STATE_Disconnected,		SMEV_ForgetDevice,				STATE_Idle,				&ForgetDevice);
	InitStateNode (STATE_Disconnected,		SMEV_SelectDevice,				STATE_Disconnected,		&SelectDevice);
	InitStateNode (STATE_Disconnected,		SMEV_Disconnect,				STATE_Disconnected,		NULLTRANS);
	InitStateNode (STATE_Disconnected,		SMEV_ConnectStart,				STATE_Connecting,		&Connect);
	InitStateNode (STATE_Disconnected,		SMEV_Timeout,					STATE_Connecting,		&Connect);
	InitStateNode (STATE_Disconnected,		SMEV_StartOutgoingCall,			STATE_Disconnected,		&IncorrectState4Call);
	InitStateNode (STATE_Disconnected,		SMEV_SwitchHeadset,				STATE_Disconnected,		&SetPcsoundPrefFlag);
	/*-------------------------------------------------------------------------------------------------*/

	/*---------------------------------- STATE: Connecting   ------------------------------------------*/
	InitStateNode (STATE_Connecting,		SMEV_ForgetDevice,				STATE_Idle,				&ForgetDevice);
	InitStateNode (STATE_Connecting,		SMEV_SelectDevice,				STATE_Disconnected,		&SelectDevice);
	InitStateNode (STATE_Connecting,		SMEV_Disconnect,				STATE_Disconnected,		&Disconnect);
	InitStateNode (STATE_Connecting,		SMEV_Failure,					STATE_Disconnected,		&ConnectFailure);
	InitStateNode (STATE_Connecting,		SMEV_Connected,					STATE_Connected,		&Connected);
	InitStateNode (STATE_Connecting,		SMEV_StartOutgoingCall,			STATE_Connecting,		&IncorrectState4Call);
	InitStateNode (STATE_Connecting,		SMEV_SwitchHeadset,				STATE_Connecting,		&SetPcsoundPrefFlag);
	/*-------------------------------------------------------------------------------------------------*/

	/*---------------------------------- STATE: Connected   -------------------------------------------*/
	InitStateNode (STATE_Connected,			SMEV_ForgetDevice,				STATE_Idle,				&ForgetDevice);
	InitStateNode (STATE_Connected,			SMEV_SelectDevice,				STATE_Disconnected,		&SelectDevice);
	InitStateNode (STATE_Connected,			SMEV_Disconnect,				STATE_Disconnected,		&Disconnect);
	InitStateNode (STATE_Connected,			SMEV_HfpConnectStart,			STATE_HfpConnecting,	&HfpConnect);
	InitStateNode (STATE_Connected,			SMEV_StartOutgoingCall,			STATE_Connected,		&IncorrectState4Call);
	InitStateNode (STATE_Connected,			SMEV_SwitchHeadset,				STATE_Connected,		&SetPcsoundPrefFlag);
	/*-------------------------------------------------------------------------------------------------*/

	/*---------------------------------- STATE: HfpConnecting   ---------------------------------------*/
	InitStateNode (STATE_HfpConnecting,		SMEV_ForgetDevice,				STATE_Idle,				&ForgetDevice);
	InitStateNode (STATE_HfpConnecting,		SMEV_SelectDevice,				STATE_Disconnected,		&SelectDevice);
	InitStateNode (STATE_HfpConnecting,		SMEV_Disconnect,				STATE_Disconnected,		&Disconnect);
	InitStateNode (STATE_HfpConnecting,		SMEV_Failure,					STATE_Disconnected,		&ServiceConnectFailure);
	InitStateNode (STATE_HfpConnecting,		SMEV_HfpConnected,				STATE_HfpConnected,		&HfpConnected);
	InitStateNode (STATE_HfpConnecting,		SMEV_Timeout,					&IsHfpConnected,		2);
		InitChoice (0, STATE_HfpConnecting,	SMEV_Timeout,					STATE_Disconnected,		&ServiceConnectFailure);
		InitChoice (1, STATE_HfpConnecting,	SMEV_Timeout,					STATE_HfpConnected,		&HfpConnected);
	InitStateNode (STATE_HfpConnecting,		SMEV_StartOutgoingCall,			STATE_HfpConnecting,	&IncorrectState4Call);
	InitStateNode (STATE_HfpConnecting,		SMEV_AtResponse,				STATE_HfpConnecting,	&AtProcessing);
	InitStateNode (STATE_HfpConnecting,		SMEV_SwitchHeadset,				STATE_HfpConnecting,	&SetPcsoundPrefFlag);
	/*------------------------------------------------------------------------------------------------*/

	/*---------------------------------- STATE: HfpConnected   ----------------------------------------*/
	InitStateNode (STATE_HfpConnected,		SMEV_ForgetDevice,				STATE_Idle,				&ForgetDevice);
	InitStateNode (STATE_HfpConnected,		SMEV_SelectDevice,				STATE_Disconnected,		&SelectDevice);
	InitStateNode (STATE_HfpConnected,		SMEV_Disconnect,				STATE_Disconnected,		&Disconnect);
	InitStateNode (STATE_HfpConnected,		SMEV_CallEnded,					STATE_HfpConnected,		NULLTRANS);
	InitStateNode (STATE_HfpConnected,		SMEV_SwitchHeadset,				STATE_HfpConnected,		&SetPcsoundPrefFlag);
	InitStateNode (STATE_HfpConnected,		SMEV_StartOutgoingCall,			STATE_Calling,			&StartOutgoingCall);
	InitStateNode (STATE_HfpConnected,		SMEV_AtResponse,				&ToRingingOrCalling,	3);
		InitChoice (0, STATE_HfpConnected,	SMEV_AtResponse,				STATE_HfpConnected,		&AtProcessing);
		InitChoice (1, STATE_HfpConnected,	SMEV_AtResponse,				STATE_Ringing,			&Ringing);
		InitChoice (2, STATE_HfpConnected,	SMEV_AtResponse,				STATE_Calling,			&CallFromPhone);
	/*-------------------------------------------------------------------------------------------------*/

	/*---------------------------------- STATE: Calling   ---------------------------------------------*/
	InitStateNode (STATE_Calling,			SMEV_Disconnect,				STATE_Disconnected,		&Disconnect);
	InitStateNode (STATE_Calling,			SMEV_Failure,					STATE_HfpConnected,		&EndCall);
	InitStateNode (STATE_Calling,			SMEV_CallEnd,					STATE_HfpConnected,		&EndCall);
	InitStateNode (STATE_Calling,			SMEV_CallEnded,					STATE_HfpConnected,		&EndCall);
	InitStateNode (STATE_Calling,			SMEV_CallStart,					STATE_InCall,			&StartCall);
	InitStateNode (STATE_Calling,			SMEV_SwitchHeadset,				STATE_Calling,			&SetPcsoundPrefFlag);
	InitStateNode (STATE_Calling,			SMEV_SwitchVoice,				STATE_Calling,			&SwitchedVoiceOnOff);
	InitStateNode (STATE_Calling,			SMEV_AtResponse,				&ChoiceCallSetup,		3);
		InitChoice (0, STATE_Calling,		SMEV_AtResponse,				STATE_HfpConnected,		&EndCall);
		InitChoice (1, STATE_Calling,		SMEV_AtResponse,				STATE_Calling,			&OutgoingCall);
		InitChoice (2, STATE_Calling,		SMEV_AtResponse,				STATE_Calling,			&AtProcessing);
	/*-------------------------------------------------------------------------------------------------*/

	/*---------------------------------- STATE: Ringing   ---------------------------------------------*/
	InitStateNode (STATE_Ringing,			SMEV_Disconnect,				STATE_Disconnected,		&Disconnect);
	InitStateNode (STATE_Ringing,			SMEV_Failure,					STATE_HfpConnected,		&EndCall);
	InitStateNode (STATE_Ringing,			SMEV_CallEnd,					STATE_HfpConnected,		&EndCall);
	InitStateNode (STATE_Ringing,			SMEV_CallEnded,					STATE_HfpConnected,		&EndCall);
	InitStateNode (STATE_Ringing,			SMEV_Answer,					STATE_Ringing,			&Answer);
	InitStateNode (STATE_Ringing,			SMEV_CallStart,					STATE_InCall,			&StartCall);
	InitStateNode (STATE_Ringing,			SMEV_SwitchHeadset,				STATE_Ringing,			&SetPcsoundPrefFlag);
	InitStateNode (STATE_Ringing,			SMEV_SwitchVoice,				STATE_Ringing,			&SwitchedVoiceOnOff);
	InitStateNode (STATE_Ringing,			SMEV_AtResponse,				&ChoiceFromRinging,		2);
		InitChoice (0, STATE_Ringing,		SMEV_AtResponse,				STATE_HfpConnected,		&EndCall);
		InitChoice (1, STATE_Ringing,		SMEV_AtResponse,				STATE_Ringing,			&AtProcessing);
	/*-------------------------------------------------------------------------------------------------*/

	/*---------------------------------- STATE: InCall  ------------------------------------------------*/
	InitStateNode (STATE_InCall,			SMEV_Disconnect,				STATE_Disconnected,		&Disconnect);
	InitStateNode (STATE_InCall,			SMEV_Failure,					STATE_InCall,			&StopVoice);
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

	UserCallback.Construct (cb);
	HfpSmObj.Construct();
	UserCallback.InitialCallback();
}


void HfpSm::End ()
{
	HfpSmObj.Destruct();
}


/********************************************************************************\
								SM Help functions
\********************************************************************************/

bool HfpSm::ParseAndSetAtIndicators (char* services)
{
	enum {
		CALL,
		CALLSETUP,
		CALLHELD,
		NumServices,
		MaxServices = 10
	};

	static STRB servtable[NumServices] = { "call", "callsetup", "callheld" };

	int   servnums [NumServices] = {0};
	char* s1, *s2;
	int   n = 0;

	STRB str(services);

	// 1st ScanCharNext instead ScanChar is ok because the first services char should be '('
	for (int i = 0;  i < MaxServices;  i++)
	{
		if ( !(s1 = str.ScanCharNext('\"')) )
			break;
		if ( !(s2 = str.ScanCharNext('\"')) )
			break;
		*s2 = '\0';
		for (int j = 0; j < NumServices; j++)
			if (servtable[j] == (cchar*)(s1+1)) {
				servnums[j] = i + 1;  // HFP indicator index starts from 1
				n++;
				break;
			}
	}

	if (n != NumServices) {
		LogMsg ("ParseAndSetAtIndicators failed");
		return false;
	}

	InHand::SetIndicatorsNumbers(servnums[CALL],servnums[CALLSETUP],servnums[CALLHELD]);
	return true;
}


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
		HfpSm::PutEvent_Failure();	// if we have an error when stopping voice, we should not to notice, it may be the normal case
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
	PublicParams.AbonentCurrent = PublicParams.AbonentWaiting = PublicParams.AbonentHeld = 0;
}


void HfpSm::SetCallInfo4CurrentCall (CallInfo<char> *info)
{
	if (CallInfoCurrent)
		delete CallInfoCurrent;

	if (info) {
		CallInfoCurrent = info;
		CallInfoCurrent->Parse2NumberName ();
		PublicParams.AbonentCurrent = CallInfoCurrent->GetAbonent();
	}
	else {
		CallInfoCurrent = 0;
		PublicParams.AbonentCurrent = 0;
	}
	UserCallback.CallCurrentInfo();
}


void HfpSm::SetCallInfo4WaitingCall (CallInfo<char> *info)
{
	if (CallInfoWaiting)
		delete CallInfoWaiting;

	if (info) {
		CallInfoWaiting = info;
		CallInfoWaiting->Parse2NumberName ();
		PublicParams.AbonentWaiting = CallInfoWaiting->GetAbonent();
	}
	else {
		CallInfoWaiting = 0;
		PublicParams.AbonentWaiting = 0;
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
			PublicParams.AbonentHeld = CallInfoHeld->GetAbonent();
			CallInfoCurrent = 0;
			PublicParams.AbonentCurrent = 0;
			addflag = DIALAPP_FLAG_ABONENT_CURRENT;
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
				PublicParams.AbonentWaiting = 0;
				PublicParams.AbonentHeld	= CallInfoHeld->GetAbonent();
				addflag = DIALAPP_FLAG_ABONENT_WAITING;
			}
			else 
			{
				// Probably we are switching held and current 
				CallInfo<char> *c = CallInfoHeld;
				CallInfoHeld = CallInfoCurrent;
				CallInfoCurrent = c;
				PublicParams.AbonentCurrent = (CallInfoCurrent) ? CallInfoCurrent->GetAbonent() : 0;
				PublicParams.AbonentHeld	= (CallInfoHeld)    ? CallInfoHeld->GetAbonent()    : 0;
				addflag = DIALAPP_FLAG_ABONENT_CURRENT;
			}
			break;

		default:
			LogMsg("Incorrect Held Status");
			return;
	}

	UserCallback.CallHeldInfo(addflag);
}


	
/********************************************************************************\
								SM TRANSITIONS
\********************************************************************************/

bool HfpSm::SetPcsoundPrefFlag (SMEVENT* ev, int param)
{
	if (WasPcsoundPrefChanged(ev))	{
		ScoAppObj->SetIncomingReadiness (PublicParams.PcSoundPref);
		if (State_next!=STATE_InCall) // there is another callback for InCall state
			UserCallback.PcSoundOnOff();
	}
	return true;
}


bool HfpSm::SwitchVoiceOnOff (SMEVENT* ev, int param)
{
	if (WasPcsoundPrefChanged(ev))	{
		if (PublicParams.PcSound = PublicParams.PcSoundPref)
			StartVoiceHlp(false);
		else
			StopVoiceHlp(false);
		UserCallback.PcSoundOnOff (DIALAPP_FLAG_PCSOUND);
	}
	return true;
}


bool HfpSm::SwitchedVoiceOnOff (SMEVENT* ev, int param)
{
	if (ev->Param.PcSound == PublicParams.PcSound) {
		// Event bringing PcSound state that is already set
		LogMsg ("WARNING: Acquiring PcSound state is already set, do nothing");
		return true;
	}

	if (PublicParams.PcSound = PublicParams.PcSoundPref = ev->Param.PcSound)
		StartVoiceHlp(true);
	else
		StopVoiceHlp(true);
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
	if (State >= STATE_HfpConnected)
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
	HfpIndicators = false;
	InHand::ClearIndicatorsNumbers();
	MyTimer.Start(TIMEOUT_HFP_NEGOTIATION,true);
	AtResponsesCnt = 0;
	AtResponsesNum = InHand::BeginHfpConnect();
	return true;
}


bool HfpSm::HfpConnected (SMEVENT* ev, int param)
{
	if (ev->Ev != SMEV_Timeout)
		MyTimer.Stop();

	try
	{
		ScoAppObj->StartServer (PublicParams.CurDevice->Address, PublicParams.PcSoundPref);
		UserCallback.HfpConnected();
	}
	catch (int err)
	{
		LogMsg("EXCEPTION %d", err);
		HfpSm::PutEvent_Disconnect(DialAppError_ServiceConnectFailure);
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
	else if (ev->Ev == SMEV_Failure)
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
	if (ev->Ev == SMEV_Failure)
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


bool HfpSm::EndCallVoiceFailure (SMEVENT* ev, int param)
{
	LogMsg ("Ending call because of voice channel problem");
	bool res = EndCall (ev, param);  // TODO: what will happens in 3-way call???
	if (ev->Param.ReportError)
		UserCallback.NotifyFailure(ev->Param.ReportError);
	// else it may be normal case of closing channel, so do not report
	ClearAllCallInfo();
	return res;
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
			// For STATE_HfpConnecting count the Ok response in order to generate Connected event
			if (State == STATE_HfpConnecting && (++AtResponsesCnt == AtResponsesNum)) {
				AtResponsesCnt = AtResponsesNum = 0;
				if (HfpIndicators)
					HfpSm::PutEvent_HfpConnected();
				else
					HfpSm::PutEvent_Failure(DialAppError_ServiceConnectFailure);
			}
			break;

		case SMEV_AtResponse_CurrentPhoneIndicators:
			// This command is expected in STATE_HfpConnecting state only
			ASSERT__ (State == STATE_HfpConnecting);
			HfpIndicators = ParseAndSetAtIndicators(ev->Param.InfoCh->Info);
			delete ev->Param.InfoCh;
			break;

		case SMEV_AtResponse_ListCurrentCalls:
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


/********************************************************************************\
								SM CHOICES
\********************************************************************************/

int HfpSm::IsHfpConnected (SMEVENT* ev)
{
	// if mandatory HFP Current Phone Indicators (+CIND:) was not received during HFP negotiation - go to 0
	return (int) HfpIndicators;
}


int HfpSm::ToRingingOrCalling (SMEVENT* ev)
{
	switch (ev->Param.AtResponse)
	{
		case SMEV_AtResponse_CallSetup_Incoming:
			LogMsg("CallSetup_Incoming");
			return 1; // to STATE_Ringing
		case SMEV_AtResponse_CallSetup_Outgoing:
			LogMsg("CallSetup_Outgoing");
			return 2; // to STATE_Ringing
	}
	return 0; // call AtProcessing
}


int HfpSm::ChoiceFromRinging (SMEVENT* ev)
{
	switch (ev->Param.AtResponse)
	{
		case SMEV_AtResponse_CallSetup_None:
			LogMsg("CallSetup_None");
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
			LogMsg("CallSetup_Outgoing");
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