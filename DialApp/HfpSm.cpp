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


void HfpSm::Construct()
{
	MyTimer.Construct ();
	SMT<HfpSm>::Construct (SM_HFP);
	ScoAppObj = new ScoApp();
	ScoAppObj->Construct();
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
	InitStateNode (STATE_Idle,				SMEV_Headset,					STATE_Idle,				&SetHeadsetFlag);
	/*-------------------------------------------------------------------------------------------------*/

	/*---------------------------------- STATE: Disconnected ------------------------------------------*/
	InitStateNode (STATE_Disconnected,		SMEV_ForgetDevice,				STATE_Idle,				&ForgetDevice);
	InitStateNode (STATE_Disconnected,		SMEV_SelectDevice,				STATE_Disconnected,		&SelectDevice);
	InitStateNode (STATE_Disconnected,		SMEV_Disconnect,				STATE_Disconnected,		NULLTRANS);
	InitStateNode (STATE_Disconnected,		SMEV_ConnectStart,				STATE_Connecting,		&Connect);
	InitStateNode (STATE_Disconnected,		SMEV_Timeout,					STATE_Connecting,		&Connect);
	InitStateNode (STATE_Disconnected,		SMEV_StartOutgoingCall,			STATE_Disconnected,		&IncorrectState4Call);
	InitStateNode (STATE_Disconnected,		SMEV_Headset,					STATE_Disconnected,		&SetHeadsetFlag);
	/*-------------------------------------------------------------------------------------------------*/

	/*---------------------------------- STATE: Connecting   ------------------------------------------*/
	InitStateNode (STATE_Connecting,		SMEV_ForgetDevice,				STATE_Idle,				&ForgetDevice);
	InitStateNode (STATE_Connecting,		SMEV_SelectDevice,				STATE_Disconnected,		&SelectDevice);
	InitStateNode (STATE_Connecting,		SMEV_Disconnect,				STATE_Disconnected,		&Disconnect);
	InitStateNode (STATE_Connecting,		SMEV_Failure,					STATE_Disconnected,		&ConnectFailure);
	InitStateNode (STATE_Connecting,		SMEV_Connected,					STATE_Connected,		&Connected);
	InitStateNode (STATE_Connecting,		SMEV_StartOutgoingCall,			STATE_Connecting,		&IncorrectState4Call);
	InitStateNode (STATE_Connecting,		SMEV_Headset,					STATE_Connecting,		&SetHeadsetFlag);
	/*-------------------------------------------------------------------------------------------------*/

	/*---------------------------------- STATE: Connected   -------------------------------------------*/
	InitStateNode (STATE_Connected,			SMEV_ForgetDevice,				STATE_Idle,				&ForgetDevice);
	InitStateNode (STATE_Connected,			SMEV_SelectDevice,				STATE_Disconnected,		&SelectDevice);
	InitStateNode (STATE_Connected,			SMEV_Disconnect,				STATE_Disconnected,		&Disconnect);
	InitStateNode (STATE_Connected,			SMEV_HfpConnectStart,			STATE_HfpConnecting,	&HfpConnect);
	InitStateNode (STATE_Connected,			SMEV_StartOutgoingCall,			STATE_Connected,		&IncorrectState4Call);
	InitStateNode (STATE_Connected,			SMEV_Headset,					STATE_Connected,		&SetHeadsetFlag);
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
	InitStateNode (STATE_HfpConnecting,		SMEV_Headset,					STATE_HfpConnecting,	&SetHeadsetFlag);
	/*------------------------------------------------------------------------------------------------*/

	/*---------------------------------- STATE: HfpConnected   ----------------------------------------*/
	InitStateNode (STATE_HfpConnected,		SMEV_ForgetDevice,				STATE_Idle,				&ForgetDevice);
	InitStateNode (STATE_HfpConnected,		SMEV_SelectDevice,				STATE_Disconnected,		&SelectDevice);
	InitStateNode (STATE_HfpConnected,		SMEV_Disconnect,				STATE_Disconnected,		&Disconnect);
	InitStateNode (STATE_HfpConnected,		SMEV_CallEnd,					STATE_HfpConnected,		NULLTRANS);
	InitStateNode (STATE_HfpConnected,		SMEV_Headset,					STATE_HfpConnected,		&SetHeadsetFlag);
	InitStateNode (STATE_HfpConnected,		SMEV_StartOutgoingCall,			STATE_Calling,			&StartOutgoingCall);
	InitStateNode (STATE_HfpConnected,		SMEV_AtResponse,				&ChoiceToRinging,		2);
		InitChoice (0, STATE_HfpConnected,	SMEV_AtResponse,				STATE_HfpConnected,		&AtProcessing);
		InitChoice (1, STATE_HfpConnected,	SMEV_AtResponse,				STATE_Ringing,			&Ringing);
	/*-------------------------------------------------------------------------------------------------*/

	/*---------------------------------- STATE: Calling   ---------------------------------------------*/
	InitStateNode (STATE_Calling,			SMEV_Disconnect,				STATE_Disconnected,		&Disconnect);
	InitStateNode (STATE_Calling,			SMEV_Failure,					STATE_HfpConnected,		&EndCall);
	InitStateNode (STATE_Calling,			SMEV_CallEnd,					STATE_HfpConnected,		&EndCall);
	InitStateNode (STATE_Calling,			SMEV_CallStart,					STATE_InCall,			&StartCall);
	InitStateNode (STATE_Calling,			SMEV_Headset,					STATE_Calling,			&SetHeadsetFlag);
	InitStateNode (STATE_Calling,			SMEV_AtResponse,				&ChoiceCallSetup,		4);
		InitChoice (0, STATE_Calling,		SMEV_AtResponse,				STATE_HfpConnected,		&EndCall);
		InitChoice (1, STATE_Calling,		SMEV_AtResponse,				STATE_HfpConnected,		&EndCall);
		InitChoice (2, STATE_Calling,		SMEV_AtResponse,				STATE_Calling,			&OutgoingCall);
		InitChoice (3, STATE_Calling,		SMEV_AtResponse,				STATE_Calling,			&AtProcessing);
	/*-------------------------------------------------------------------------------------------------*/

	/*---------------------------------- STATE: Ringing   ---------------------------------------------*/
	InitStateNode (STATE_Ringing,			SMEV_Disconnect,				STATE_Disconnected,		&Disconnect);
	InitStateNode (STATE_Ringing,			SMEV_Failure,					STATE_HfpConnected,		&EndCall);
	InitStateNode (STATE_Ringing,			SMEV_CallEnd,					STATE_HfpConnected,		&EndCall);
	InitStateNode (STATE_Ringing,			SMEV_Answer,					STATE_Ringing,			&Answer);
	InitStateNode (STATE_Ringing,			SMEV_CallStart,					STATE_InCall,			&StartCall);
	InitStateNode (STATE_Ringing,			SMEV_AtResponse,				&ChoiceFromRinging,		2);
		InitChoice (0, STATE_Ringing,		SMEV_AtResponse,				STATE_HfpConnected,		&EndCall);
		InitChoice (1, STATE_Ringing,		SMEV_AtResponse,				STATE_Ringing,			&AtProcessing);
	
	//TODO: to think how to precess here SMEV_Headset
	/*-------------------------------------------------------------------------------------------------*/

	/*---------------------------------- STATE: InCall  ------------------------------------------------*/
	InitStateNode (STATE_InCall,			SMEV_Disconnect,				STATE_Disconnected,		&Disconnect);
	InitStateNode (STATE_InCall,			SMEV_Failure,					STATE_HfpConnected,		&EndCallVoiceFailure);
	InitStateNode (STATE_InCall,			SMEV_CallEnd,					STATE_HfpConnected,		&EndCall);
	InitStateNode (STATE_InCall,			SMEV_AtResponse,				STATE_InCall,			&AtProcessing);
	InitStateNode (STATE_InCall,			SMEV_SendDtmf,					STATE_InCall,			&SendDtmf);
//	InitStateNode (STATE_InCall,			SMEV_PutOnHold,					STATE_InCall,			&PutOnHold);
//	InitStateNode (STATE_InCall,			SMEV_IncomingCall,				STATE_InCall,			&Ringing);
//	InitStateNode (STATE_InCall,			SMEV_CallWaiting,				STATE_InCall,			&PutOnHold);
	InitStateNode (STATE_InCall,			SMEV_Headset,					STATE_InCall,			&SwitchVoiceOnOff);
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


bool HfpSm::WasPcsoundChanged (SMEVENT* ev)
{
	if (PublicParams.PcSound != ev->Param.HeadsetOn) {
		PublicParams.PcSound = ev->Param.HeadsetOn;
		LogMsg ("Headset = %d", PublicParams.PcSound);
		return true;
	}
	return false;
}


void HfpSm::StartVoiceHlp ()
{
	LogMsg ("Starting voice...");
	try
	{
		ScoAppObj->OpenSco (PublicParams.CurDevice->Address);
		ScoAppObj->VoiceStart();
		PublicParams.PcSoundNowOn = true;
	}
	catch (int err)
	{
		LogMsg("EXCEPTION %d", err);
		HfpSm::PutEvent_Failure2Report();	// if we have an error when starting voice, we should to notice
	}
}


void HfpSm::StopVoiceHlp ()
{
	LogMsg ("Stopping voice...");
	try
	{
		ScoAppObj->CloseSco();
	}
	catch (int err)
	{
		LogMsg("EXCEPTION %d", err);
		HfpSm::PutEvent_Failure();	// if we have an error when stopping voice, we should not to notice, it may be the normal case
	}
	PublicParams.PcSoundNowOn = false;
}

bool HfpSm::ParseCurrentCalls(char* CurrentCall, DialAppParam* param)
{
	param->AbonentName   = 0;
	param->AbonentNumber = 0;

	STRB str(CurrentCall);

	// Search for "Number"
	char* s1 = str.ScanChar('\"');
	if (!s1)
		return false;
	char* s2 = str.ScanCharNext('\"');
	if (!s2)
		return false;
	*s2 = '\0';	
	param->AbonentNumber = s1 + 1;

	// Search for "Name"
	if (s1 = str.ScanCharNext('\"'))
	{
		s2 = str.ScanCharNext('\"');
		if (!s2)
			return false;
		*s2 = '\0';	
		param->AbonentName = s1 + 1;
	}
	return true;
}

	
/********************************************************************************\
								SM TRANSITIONS
\********************************************************************************/

bool HfpSm::SetHeadsetFlag (SMEVENT* ev, int param)
{
	if (WasPcsoundChanged(ev))	{
		// There is another callback for InCall states
		if (State_next!=STATE_InCall)
			UserCallback.HedasetOnOff();
	}
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
		UserCallback.DeviceUnknown ();
		goto end_func;
	}

	if (State > STATE_Disconnected)
		InHand::Disconnect ();

	LogMsg ("Selected device: %llX, %s", addr, dev->Name);
	PublicParams.CurDevice = dev;
	UserCallback.DevicePresent ();
	MyTimer.Start(TIMEOUT_CONNECTION_POLLING,true);
	PutEvent_ConnectStart(addr);

	end_func:
	return true;
}


bool HfpSm::ForgetDevice (SMEVENT* ev, int param)
{
	MyTimer.Stop();	
	if (State > STATE_Disconnected)
		InHand::Disconnect ();
	void * prevdev = PublicParams.CurDevice;
	PublicParams.CurDevice = 0;
	// Report about forgot device in the case it was present before, 
	// otherwise it may be the case of failure in HfpSm::SelectDevice in Idle state
	if (prevdev)
		UserCallback.DeviceForgot();
	return true;
}


bool HfpSm::Disconnect (SMEVENT* ev, int param)
{
	if (State > STATE_Disconnected)
		InHand::Disconnect ();
	MyTimer.Start(TIMEOUT_CONNECTION_POLLING,true);
	UserCallback.DevicePresent (0);
	return true;
}


bool HfpSm::Connect (SMEVENT* ev, int param)
{
	MyTimer.Start(TIMEOUT_CONNECTION_POLLING,true);
	InHand::BeginConnect(PublicParams.CurDevice->Address);
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
	UserCallback.HfpConnected();
	return true;
}


bool HfpSm::ConnectFailure (SMEVENT* ev, int param)
{
	MyTimer.Start(TIMEOUT_CONNECTION_POLLING,true);
	UserCallback.ConnectFailure();
	return true;
}


bool HfpSm::ServiceConnectFailure (SMEVENT* ev, int param)
{
	InHand::Disconnect ();
	// Here must return to Disconnected state 
	MyTimer.Start(TIMEOUT_CONNECTION_POLLING,true);
	UserCallback.ServiceConnectFailure();
	return true;
}


bool HfpSm::EndCall (SMEVENT* ev, int param)
{
	LogMsg ("Ending call...");
	InHand::EndCall();
	if (PublicParams.PcSoundNowOn)
		StopVoiceHlp ();
	if (ev->Ev == SMEV_Failure)
		UserCallback.CallFailure();
	else
		UserCallback.CallEnded();
	return true;
}


bool HfpSm::IncorrectState4Call (SMEVENT* ev, int param)
{
	LogMsg ("IncorrectState4Call !!");
	UserCallback.OutgoingIncorrectState();
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
	if (PublicParams.PcSound)
		StartVoiceHlp();
	InHand::ListCurrentCalls();
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
	return true;
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


bool HfpSm::SwitchVoiceOnOff (SMEVENT* ev, int param)
{
	if (WasPcsoundChanged(ev))	{
		if (PublicParams.PcSound)
			StartVoiceHlp();
		else
			StopVoiceHlp();
		UserCallback.InCallHeadsetOnOff();
	}
	return true;
}


bool HfpSm::Ringing (SMEVENT* ev, int param)
{
	if (PublicParams.PcSound)
		StartVoiceHlp();
	UserCallback.Ring ();
	InHand::ListCurrentCalls();
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
					HfpSm::PutEvent_Failure();
			}
			break;

		case SMEV_AtResponse_CurrentPhoneIndicators:
			// This command is expected in STATE_HfpConnecting state only
			ASSERT__ (State == STATE_HfpConnecting);
			HfpIndicators = ParseAndSetAtIndicators(ev->Param.InfoCh->Info);
			delete ev->Param.InfoCh;
			break;

		case SMEV_AtResponse_ListCurrentCalls:
			ParseCurrentCalls(ev->Param.InfoCh->Info, &PublicParams);
			UserCallback.AbonentInfo();
			delete ev->Param.InfoCh;
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
	InHand::PutOnHold();
	return true;
}


bool HfpSm::ActivateOnHoldCall(SMEVENT* ev, int param)
{
	InHand::ActivateOnHoldCall(param);
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


int HfpSm::ChoiceToRinging (SMEVENT* ev)
{
	switch (ev->Param.AtResponse)
	{
		case SMEV_AtResponse_CallSetup_Incoming:
			LogMsg("CallSetup_Incoming");
			return 1; // to STATE_Ringing
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
			return 0;	// call failure, back to STATE_HfpConnected
		case SMEV_AtResponse_CallSetup_None:
			return 1;	// call terminated, back to STATE_HfpConnected
		case SMEV_AtResponse_CallSetup_Outgoing:
			LogMsg("CallSetup_Outgoing");
			return 2;	// OutgoingCall
	}
	return 3;	// stay in STATE_Calling, call AtProcessing
}
