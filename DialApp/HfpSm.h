/*******************************************************************\
 Filename    :  HfpSm.h
 Purpose     :  Main HFP State Machine
\*******************************************************************/

#ifndef _HFPSM_H_
#define _HFPSM_H_

#include "DialAppType.h"
#include "mutex.h"
#include "smBase.h"
#include "smTimer.h"
#include "InHand.h"
#include "ScoApp.h"
#include "CallInfo.h"


#define STATE_LIST_HFPSM	\
    STATE (Init				),		\
    STATE (Idle				),		\
    STATE (Disconnected		),		\
    STATE (Connecting		),		\
    STATE (Connected		),		\
    STATE (HfpConnecting	),		\
    STATE (HfpConnected		),		\
    STATE (Calling			),		\
    STATE (Ringing			),		\
    STATE (InCall			)


class HfpSmCb
{
  public:
	HfpSmCb () {};
	HfpSmCb (DialAppCb cb) { Construct(cb); }

	void Construct (DialAppCb cb) { CbFunc = cb; }

  public:
	void InitialCallback		();
	void DevicePresent			(uint32 addflag = DIALAPP_FLAG_CURDEV);
	void DeviceUnknown			();
	void DeviceForgot			();
	void NotifyFailure			(int error);
	void HfpConnected			();
	void Calling				();
	void PcSoundOnOff			(uint32 addflag = 0);
	void PcSoundOff				();
	void InCall					();
	void CallEnded				();
	void Ring					();
	void CallCurrentInfo		();
	void CallWaitingInfo		();
	void CallHeldInfo			(uint32 addflag = 0);

  protected:
	DialAppCb	CbFunc;
};


// class-style struct to help setting DialAppParam fields
struct HfpPublicParam: public DialAppParam
{
	void SetAbonentCurrent	(CallInfo<char> * info) { AbonentCurrent = (info) ? info->GetAbonent():0; }
	void SetAbonentWaiting	(CallInfo<char> * info) { AbonentWaiting = (info) ? info->GetAbonent():0; }
	void SetAbonentHeld		(CallInfo<char> * info) { AbonentHeld	 = (info) ? info->GetAbonent():0; }

	void ClearAbonentCurrent () { AbonentCurrent = 0; }
	void ClearAbonentWaiting () { AbonentWaiting = 0; }
	void ClearAbonentHeld	 () { AbonentHeld	 = 0; }
};


// Object passed to HfpSm::Init to receive result asynchronously
struct HfpSmInitReturn
{
	Event	SignalEvent;
	int		RetCode;
};


// Main HFP State Machine
class HfpSm: public SMT<HfpSm>
{
	friend class HfpSmCb;

    DECL_STATES (STATE_LIST_HFPSM)

  public:
	enum {
		// Timeouts in milliseconds
		TIMEOUT_CONNECTION_POLLING		= 5000,
		TIMEOUT_HFP_NEGOTIATION			= 1500,
		TIMEOUT_WAITING_HOLD_SWITCH		=   20,
		TIMEOUT_SCO_TREATED_AS_INCOMING	=  750,	// The Max observed value is at HTC Diamond = 0.62 sec
	};

  public:
	static void Init(DialAppCb cb, HfpSmInitReturn* initevent);
	static void End();

  protected:
	static HfpSmCb				UserCallback;
	static HfpSmInitReturn *	InitEvent;

  public:
	HfpSm(): SMT<HfpSm>("HfpSm  "), MyTimer(SM_HFP, SMEV_Timeout), ScoAppObj(0), CallInfoCurrent(0), CallInfoHeld(0), CallInfoWaiting(0), InitEventsCnt(0)
	{
		memset (&PublicParams, 0, sizeof(DialAppParam));
	}

	void Construct();
	void Destruct();

  public:
	HfpPublicParam	 PublicParams;

  public:
	SmTimer		MyTimer;
	ScoApp*		ScoAppObj;
	int			HfpIndicatorsState;			// its type is STATE or -1 meaning HfpConnected state is not achieved 
	unsigned    IncallStartTime;
	unsigned    InitEventsCnt;

	CallInfo<char>   *CallInfoCurrent;		// Set in InCall state after the abonent information is present
	CallInfo<char>   *CallInfoHeld;			// Set in InCall state when the Current call is turned to Held, it is indication about Held call presence
	CallInfo<char>   *CallInfoWaiting;		// Set in InCall state after incoming waiting call received, it is indication about Waiting call presence

  public:
	static void PutEvent_Ok ()
	{
		SMEVENT Event = {SM_HFP, SMEV_Error};
		Event.Param.ReportError = 0;
		SmBase::PutEvent (&Event, SMQ_HIGH);
	}

	static void PutEvent_Failure (int error)
	{
		::LogMsg ("Putting Failure Event #%d", error);
		SMEVENT Event = {SM_HFP, SMEV_Error};
		Event.Param.ReportError = error;
		SmBase::PutEvent (&Event, SMQ_HIGH);
	}

	static void PutEvent_SelectDevice (uint64 addr)
	{
		SMEVENT Event = {SM_HFP, SMEV_SelectDevice};
		Event.Param.BthAddr = addr;
		SmBase::PutEvent (&Event, SMQ_LOW);
	}

	static void PutEvent_ForgetDevice ()
	{
		SMEVENT Event = {SM_HFP, SMEV_ForgetDevice};
		SmBase::PutEvent (&Event, SMQ_LOW);
	}
	
	static void PutEvent_ConnectStart (uint64 addr)
	{
		SMEVENT Event = {SM_HFP, SMEV_ConnectStart};
		Event.Param.BthAddr = addr;
		SmBase::PutEvent (&Event, SMQ_LOW);
	}

	static void PutEvent_Disconnect (int error = 0)
	{
		SMEVENT Event = {SM_HFP, SMEV_Disconnect};
		Event.Param.ReportError = error;
		SmBase::PutEvent (&Event, SMQ_HIGH);
	}

	static void PutEvent_Connected ()
	{
		SMEVENT Event = {SM_HFP, SMEV_Connected};
		SmBase::PutEvent (&Event, SMQ_LOW);
	}

	static void PutEvent_HfpConnectStart ()
	{
		SMEVENT Event = {SM_HFP, SMEV_HfpConnectStart};
		SmBase::PutEvent (&Event, SMQ_LOW);
	}

	static void PutEvent_Headset (bool headset_on)
	{
		SMEVENT Event = {SM_HFP, SMEV_SwitchHeadset};
		Event.Param.PcSound = headset_on;
		SmBase::PutEvent (&Event, SMQ_HIGH);
	}

	static void PutEvent_StartOutgoingCall (cchar* dialnumber)
	{
		SMEVENT Event = {SM_HFP, SMEV_StartOutgoingCall};
		Event.Param.CallNumber = new ((char*)dialnumber) CallInfo<char>((char*)dialnumber);
		SmBase::PutEvent (&Event, SMQ_LOW);
	}

	static void PutEvent_Answer ()
	{
		SMEVENT Event = {SM_HFP, SMEV_Answer};
		SmBase::PutEvent (&Event, SMQ_HIGH);
	}

	static void PutEvent_AtResponse (SMEV_ATRESPONSE resp)
	{
		SMEVENT Event = {SM_HFP, SMEV_AtResponse};
		Event.Param.AtResponse = resp;
		SmBase::PutEvent (&Event, SMQ_HIGH);
	}

	static void PutEvent_AtResponse (SMEV_ATRESPONSE resp, int state)
	{
		SMEVENT Event = {SM_HFP, SMEV_AtResponse};
		Event.Param.AtResponse = resp;
		Event.Param.IndicatorsState = state;
		SmBase::PutEvent (&Event, SMQ_HIGH);
	}

	static void PutEvent_AtResponse (SMEV_ATRESPONSE resp, char* info)
	{
		SMEVENT Event = {SM_HFP, SMEV_AtResponse};
		Event.Param.AtResponse = resp;
		Event.Param.InfoCh	   = new (info) CallInfo<char>(info);
		SmBase::PutEvent (&Event, SMQ_LOW);
	}

	static void PutEvent_AtResponse (SMEV_ATRESPONSE resp, wchar* info)
	{
		SMEVENT Event = {SM_HFP, SMEV_AtResponse};
		Event.Param.AtResponse = resp;
		Event.Param.InfoWch	   = new (info) CallInfo<wchar>(info);
		SmBase::PutEvent (&Event, SMQ_LOW);
	}

	static void PutEvent_CallEnd ()
	{
		SMEVENT Event = {SM_HFP, SMEV_CallEnd};
		SmBase::PutEvent (&Event, SMQ_HIGH);
	}

	static void PutEvent_CallEnded ()
	{
		SMEVENT Event = {SM_HFP, SMEV_CallEnded};
		SmBase::PutEvent (&Event, SMQ_HIGH);
	}

	static void PutEvent_CallStart ()
	{
		SMEVENT Event = {SM_HFP, SMEV_CallStart};
		SmBase::PutEvent (&Event, SMQ_HIGH);
	}

	static void PutEvent_SendDtmf (cchar dialinfo)
	{
		SMEVENT Event = {SM_HFP, SMEV_SendDtmf};
		Event.Param.Dtmf = dialinfo;
		SmBase::PutEvent (&Event, SMQ_LOW);
	}

	static void PutEvent_PutOnHold ()
	{
		SMEVENT Event = {SM_HFP, SMEV_PutOnHold};
		SmBase::PutEvent (&Event, SMQ_LOW);
	}

	static void PutEvent_CallWaiting(char* info)
	{
		SMEVENT Event = {SM_HFP, SMEV_CallWaiting};
		Event.Param.AtResponse = SMEV_AtResponse_CallWaiting_Ringing;
		Event.Param.InfoCh = new (info) CallInfo<char>(info);
		SmBase::PutEvent (&Event, SMQ_HIGH);
	}

	static void PutEvent_CallWaitingStopped()
	{
		SMEVENT Event = {SM_HFP, SMEV_CallWaiting};
		Event.Param.AtResponse = SMEV_AtResponse_CallWaiting_Stopped;
		SmBase::PutEvent (&Event, SMQ_LOW);	// low queue, SMEV_CallHeld must be first 
	}

	static void PutEvent_CallHeld (SMEV_ATRESPONSE resp)
	{
		SMEVENT Event = {SM_HFP, SMEV_CallHeld};
		Event.Param.AtResponse = resp;
		SmBase::PutEvent (&Event, SMQ_HIGH);
	}
	
  // SCO App callbacks
  private:
	static void ScoConnectCallback ();
	static void ScoDisconnectCallback ();
	static void ScoCritErrorCallback ();

  // Help functions
  private:
	bool WasPcsoundPrefChanged (SMEVENT* ev);
	void StartVoiceHlp	(bool waveonly = true);
	void StopVoiceHlp	(bool waveonly = true);
	void SetCallInfo4CurrentCall (CallInfo<char> *info);
	void SetCallInfo4WaitingCall (CallInfo<char> *info);
	void SetCallInfo4HeldCall    (SMEV_ATRESPONSE heldstatus);
	void ClearAllCallInfo ();
	bool IsCurStateSupportingVoiceSwitch();

  // Transitions
  private:
	bool ForgetDevice				(SMEVENT* ev, int param);
	bool SelectDevice				(SMEVENT* ev, int param);
	bool Disconnect					(SMEVENT* ev, int param);
	bool Connect					(SMEVENT* ev, int param);
	bool Connected					(SMEVENT* ev, int param);
	bool HfpConnect					(SMEVENT* ev, int param);
	bool HfpConnected				(SMEVENT* ev, int param);
	bool AtProcessing				(SMEVENT* ev, int param);
	bool IncorrectState4Call		(SMEVENT* ev, int param);
	bool StartOutgoingCall			(SMEVENT* ev, int param);
	bool CallFromPhone				(SMEVENT* ev, int param);
	bool Answer						(SMEVENT* ev, int param);
	bool Answer2Waiting				(SMEVENT* ev, int param);
	bool OutgoingCall				(SMEVENT* ev, int param);
	bool StartCall					(SMEVENT* ev, int param);
	bool EndCall					(SMEVENT* ev, int param);
	bool StartCallEnding			(SMEVENT* ev, int param);
	bool FinalizeCallEnding			(SMEVENT* ev, int param);
	bool SwitchVoiceOnOff			(SMEVENT* ev, int param);
	bool SwitchedVoiceOnOff			(SMEVENT* ev, int param);
	bool StopVoice					(SMEVENT* ev, int param);
	bool RejectVoice				(SMEVENT* ev, int param);
	bool ConnectFailure				(SMEVENT* ev, int param);
	bool ServiceConnectFailure		(SMEVENT* ev, int param);
	bool Ringing					(SMEVENT* ev, int param);
	bool SendDtmf					(SMEVENT* ev, int param);
	bool PutOnHold					(SMEVENT* ev, int param);
	bool IncomingWaitingCall		(SMEVENT* ev, int param);
	bool SendWaitingCallStop		(SMEVENT* ev, int param);
	bool CallHeld		  			(SMEVENT* ev, int param);
	bool HfpConnected_CallFromPhone (SMEVENT* ev, int param);
	bool HfpConnected_Ringing		(SMEVENT* ev, int param);
	bool HfpConnected_StartCall		(SMEVENT* ev, int param);


  // Choices
  private:
	int  ChoiceCallSetup	 (SMEVENT* ev);
	int  ToRingingOrCalling	 (SMEVENT* ev);
	int  ChoiceFromRinging	 (SMEVENT* ev);
	int  ChoiceIncomingVoice (SMEVENT* ev);
	int  IsHfpConnectLastCmd (SMEVENT* ev);
	int  ChoiceProcessInit	 (SMEVENT* ev);
};


extern HfpSm HfpSmObj;


inline void HfpSmCb::InitialCallback ()
{
	CbFunc (DialAppState_IdleNoDevice, DialAppError_Ok, DIALAPP_FLAG_INITSTATE | DIALAPP_FLAG_NEWSTATE | DIALAPP_FLAG_CURDEV, &HfpSmObj.PublicParams);
}

inline void HfpSmCb::DevicePresent (uint32 addflag)
{
	uint32 flag = (HfpSmObj.State_next != HfpSmObj.State) ? DIALAPP_FLAG_NEWSTATE:0;
	CbFunc (DialAppState(HfpSmObj.State_next), DialAppError_Ok, addflag|flag, &HfpSmObj.PublicParams);
}

inline void HfpSmCb::DeviceUnknown ()
{
	uint32 flag = (HfpSmObj.State_next != HfpSmObj.State) ? DIALAPP_FLAG_NEWSTATE:0;
	CbFunc (DialAppState(HfpSmObj.State_next), DialAppError_UnknownDevice, DIALAPP_FLAG_CURDEV|flag, &HfpSmObj.PublicParams);
}

inline void HfpSmCb::DeviceForgot ()
{
	uint32 flag = (HfpSmObj.State_next != HfpSmObj.State) ? DIALAPP_FLAG_NEWSTATE:0;
	CbFunc (DialAppState_IdleNoDevice, DialAppError_Ok, DIALAPP_FLAG_CURDEV|flag, &HfpSmObj.PublicParams);
}

inline void HfpSmCb::NotifyFailure (int error)
{
	uint32 flag = (HfpSmObj.State_next != HfpSmObj.State) ? DIALAPP_FLAG_NEWSTATE:0;
	CbFunc (DialAppState(HfpSmObj.State_next), DialAppError(error), flag, &HfpSmObj.PublicParams);
	LogMsg ("ERROR %d Reported To User", error);
}

inline void HfpSmCb::HfpConnected ()
{
	uint32 flag = (HfpSmObj.State_next != HfpSmObj.State) ? DIALAPP_FLAG_NEWSTATE:0;
	CbFunc (DialAppState_ServiceConnected, DialAppError_Ok, flag, &HfpSmObj.PublicParams);
}

inline void HfpSmCb::InCall()
{
	CbFunc (DialAppState(HfpSmObj.State_next), DialAppError_Ok, DIALAPP_FLAG_NEWSTATE|DIALAPP_FLAG_PCSOUND, &HfpSmObj.PublicParams);
}

inline void HfpSmCb::PcSoundOnOff(uint32 addflag)
{
	// DIALAPP_FLAG_NEWSTATE in this case is impossible
	CbFunc (DialAppState(HfpSmObj.State_next), DialAppError_Ok, DIALAPP_FLAG_PCSOUND_PREFERENCE|addflag, &HfpSmObj.PublicParams);
}

inline void HfpSmCb::PcSoundOff()
{
	// DIALAPP_FLAG_NEWSTATE in this case is impossible
	CbFunc (DialAppState(HfpSmObj.State_next), DialAppError_Ok, DIALAPP_FLAG_PCSOUND, &HfpSmObj.PublicParams);
}

inline void HfpSmCb::CallEnded()
{
	uint32 flag = (HfpSmObj.State_next != HfpSmObj.State) ? DIALAPP_FLAG_NEWSTATE:0;
	CbFunc (DialAppState_ServiceConnected, DialAppError_Ok, flag, &HfpSmObj.PublicParams);
}

inline void HfpSmCb::Calling ()
{
	uint32 flag = (HfpSmObj.State_next != HfpSmObj.State) ? DIALAPP_FLAG_NEWSTATE:0;
	CbFunc (DialAppState_Calling, DialAppError_Ok, flag, 0);
}

inline void HfpSmCb::Ring ()
{
	uint32 flag = (HfpSmObj.State_next != HfpSmObj.State) ? DIALAPP_FLAG_NEWSTATE:0;
	if (HfpSmObj.PublicParams.PcSound)
		flag |= DIALAPP_FLAG_PCSOUND;
	CbFunc (DialAppState_Ringing, DialAppError_Ok, flag, &HfpSmObj.PublicParams);
}

inline void HfpSmCb::CallCurrentInfo ()
{
	uint32 flag = (HfpSmObj.State_next != HfpSmObj.State) ? DIALAPP_FLAG_NEWSTATE:0;
	CbFunc (DialAppState(HfpSmObj.State), DialAppError_Ok, DIALAPP_FLAG_ABONENT_CURRENT|flag, &HfpSmObj.PublicParams);
}

inline void HfpSmCb::CallWaitingInfo ()
{
	uint32 flag = (HfpSmObj.State_next != HfpSmObj.State) ? DIALAPP_FLAG_NEWSTATE:0;
	flag |= DIALAPP_FLAG_ABONENT_WAITING;
	CbFunc (DialAppState(HfpSmObj.State), DialAppError_Ok, flag, &HfpSmObj.PublicParams);
}

inline void HfpSmCb::CallHeldInfo (uint32 addflag)
{
	uint32 flag = (HfpSmObj.State_next != HfpSmObj.State) ? DIALAPP_FLAG_NEWSTATE:0;
	flag |= (DIALAPP_FLAG_ABONENT_HELD | addflag);
	CbFunc (DialAppState(HfpSmObj.State), DialAppError_Ok, flag, &HfpSmObj.PublicParams);
}


#endif  // _HFPSM_H_
