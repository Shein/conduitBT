/*******************************************************************\
 Filename    :  HfpSm.h
 Purpose     :  Main HFP State Machine
\*******************************************************************/

#ifndef _HFPSM_H_
#define _HFPSM_H_

#include "DialAppType.h"
#include "smBase.h"
#include "smTimer.h"
#include "InHand.h"
#include "ScoApp.h"
#include "CallInfo.h"


#define STATE_LIST_HFPSM	\
    STATE (Idle				),		\
    STATE (Disconnected		),		\
    STATE (Connecting		),		\
    STATE (Connected		),		\
    STATE (HfpConnecting	),		\
    STATE (HfpConnected		),		\
    STATE (Calling			),		\
    STATE (Ringing			),		\
    STATE (InCallHeadsetOff	),		\
	STATE (InCallHeadsetOn	)


class HfpSmCb
{
  public:
	HfpSmCb () {};
	HfpSmCb (DialAppCb cb) { Construct(cb); }

	void Construct (DialAppCb cb) { CbFunc = cb; }

  public:
	void InitialCallback		();
	void HedasetOnOff			();
	void DevicePresent			();
	void DeviceUnknown			();
	void DeviceForgot			();
	void HfpConnected			();
	void OutgoingIncorrectState	();
	void Calling				();
	void InCallHeadsetOnOff		(bool sound_changed = false);
	void CallEnded				();
	void ConnectFailure			();
	void ServiceConnectFailure	();
	void CallFailure			();
	void CallEndedVoiceFailure	();
	void AbonentInfo			();
	void Ring					();

  protected:
	DialAppCb	CbFunc;
};



class HfpSm: public SMT<HfpSm>
{
    DECL_STATES (STATE_LIST_HFPSM)

  public:
	enum {
		// Timeouts in milliseconds
		TIMEOUT_CONNECTION_POLLING	= 5000,
		TIMEOUT_HFP_NEGOTIATION		=  500,
	};

  public:
	static void Init(DialAppCb cb);
	static void End();

  protected:
	static HfpSmCb   UserCallback;

  public:
	HfpSm(): SMT<HfpSm>("HfpSm  "), MyTimer(SM_HFP, SMEV_Timeout), ScoAppObj(0) 
	{
		memset (&PublicParams, 0, sizeof(DialAppParam));
	}

	void Construct();
	void Destruct();

  public:
	DialAppParam	PublicParams;

  public:
	SmTimer		MyTimer;
	ScoApp*		ScoAppObj;
	int			AtResponsesCnt;
	int			AtResponsesNum;
	bool		HfpIndicators;

  public:
	static void PutEvent_Failure ()
	{
		SMEVENT Event = {SM_HFP, SMEV_Failure};
		SmBase::PutEvent (&Event, SMQ_HIGH);
	}

	static void PutEvent_Failure2Report ()
	{
		SMEVENT Event = {SM_HFP, SMEV_Failure};
		Event.Param.ReportFailure = true;
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

	static void PutEvent_Disconnect ()
	{
		SMEVENT Event = {SM_HFP, SMEV_Disconnect};
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

	static void PutEvent_HfpConnected ()
	{
		SMEVENT Event = {SM_HFP, SMEV_HfpConnected};
		SmBase::PutEvent (&Event, SMQ_LOW);
	}

	static void PutEvent_Headset (bool headset_on)
	{
		SMEVENT Event = {SM_HFP, SMEV_Headset};
		Event.Param.HeadsetOn = headset_on;
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
		SmBase::PutEvent (&Event, SMQ_LOW);
	}

	static void PutEvent_AtResponse (SMEV_ATRESPONSE resp)
	{
		SMEVENT Event = {SM_HFP, SMEV_AtResponse};
		Event.Param.AtResponse = resp;
		SmBase::PutEvent (&Event, SMQ_HIGH);
	}

	static void PutEvent_AtResponse (SMEV_ATRESPONSE resp, char* info)
	{
		SMEVENT Event = {SM_HFP, SMEV_AtResponse};
		Event.Param.AtResponse = resp;
		Event.Param.InfoCh	   = new (info) CallInfo<char>(info);
		SmBase::PutEvent (&Event, SMQ_HIGH);
	}

	static void PutEvent_AtResponse (SMEV_ATRESPONSE resp, wchar* info)
	{
		SMEVENT Event = {SM_HFP, SMEV_AtResponse};
		Event.Param.AtResponse = resp;
		Event.Param.InfoWch	   = new (info) CallInfo<wchar>(info);
		SmBase::PutEvent (&Event, SMQ_HIGH);
	}

	static void PutEvent_IncomingCall ()
	{
		SMEVENT Event = {SM_HFP, SMEV_IncomingCall};
		SmBase::PutEvent (&Event, SMQ_LOW);
	}	
	
	static void PutEvent_EndCall ()
	{
		SMEVENT Event = {SM_HFP, SMEV_EndCall};
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

	
  // Help functions
  private:
	bool ParseAndSetAtIndicators (char* services);
	bool WasPcsoundChanged (SMEVENT* ev);
	void StartVoice	();
	void StopVoice	();

  // Transitions
  private:
	bool SetHeadsetFlag		  (SMEVENT* ev, int param);
	bool ForgetDevice		  (SMEVENT* ev, int param);
	bool SelectDevice		  (SMEVENT* ev, int param);
	bool Disconnect			  (SMEVENT* ev, int param);
	bool Connect			  (SMEVENT* ev, int param);
	bool Connected			  (SMEVENT* ev, int param);
	bool HfpConnect			  (SMEVENT* ev, int param);
	bool HfpConnected		  (SMEVENT* ev, int param);
	bool AtProcessing		  (SMEVENT* ev, int param);
	bool IncorrectState4Call  (SMEVENT* ev, int param);
	bool OutgoingCall		  (SMEVENT* ev, int param);
	bool StartCall			  (SMEVENT* ev, int param);
	bool EndCall			  (SMEVENT* ev, int param);
	bool ToHeadsetOn		  (SMEVENT* ev, int param);
	bool ToHeadsetOff		  (SMEVENT* ev, int param);
	bool ConnectFailure		  (SMEVENT* ev, int param);
	bool ServiceConnectFailure(SMEVENT* ev, int param);
	bool CallFailure		  (SMEVENT* ev, int param);
	bool EndCallVoiceFailure  (SMEVENT* ev, int param);
	bool Ringing			  (SMEVENT* ev, int param);
	bool RingingCallSetup	  (SMEVENT* ev, int param);
	bool SendDtmf			  (SMEVENT* ev, int param);
	bool PutOnHold			  (SMEVENT* ev, int param);
	bool ActivateOnHoldCall	  (SMEVENT* ev, int param);
		
  // Choices
  private:
	int  IsHfpConnected	(SMEVENT* ev);
	int  GetVoiceState1	(SMEVENT* ev);
	int  GetVoiceState2	(SMEVENT* ev);
	int  GetVoiceState3	(SMEVENT* ev);
};


extern HfpSm HfpSmObj;


inline void HfpSmCb::InitialCallback ()
{
	CbFunc (DialAppState_IdleNoDevice, DialAppError_Ok, DIALAPP_FLAG_INITSTATE|DIALAPP_FLAG_CURDEV, &HfpSmObj.PublicParams);
}


inline void HfpSmCb::HedasetOnOff ()
{
	uint32 stflag = (HfpSmObj.State_next != HfpSmObj.State) ? DIALAPP_FLAG_NEWSTATE:0;
	CbFunc (DialAppState(HfpSmObj.State_next), DialAppError_Ok, DIALAPP_FLAG_PCSOUND|stflag, &HfpSmObj.PublicParams);
}

inline void HfpSmCb::DevicePresent ()
{
	uint32 stflag = (HfpSmObj.State_next != HfpSmObj.State) ? DIALAPP_FLAG_NEWSTATE:0;
	CbFunc (DialAppState(HfpSmObj.State_next), DialAppError_Ok, DIALAPP_FLAG_CURDEV|stflag, &HfpSmObj.PublicParams);
}

inline void HfpSmCb::DeviceUnknown ()
{
	uint32 stflag = (HfpSmObj.State_next != HfpSmObj.State) ? DIALAPP_FLAG_NEWSTATE:0;
	CbFunc (DialAppState(HfpSmObj.State_next), DialAppError_UnknownDevice, DIALAPP_FLAG_CURDEV|stflag, &HfpSmObj.PublicParams);
}

inline void HfpSmCb::DeviceForgot ()
{
	uint32 stflag = (HfpSmObj.State_next != HfpSmObj.State) ? DIALAPP_FLAG_NEWSTATE:0;
	CbFunc (DialAppState_IdleNoDevice, DialAppError_Ok, DIALAPP_FLAG_CURDEV|stflag, &HfpSmObj.PublicParams);
}

inline void HfpSmCb::HfpConnected ()
{
	uint32 stflag = (HfpSmObj.State_next != HfpSmObj.State) ? DIALAPP_FLAG_NEWSTATE:0;
	CbFunc (DialAppState_ServiceConnected, DialAppError_Ok, stflag, 0);
}

inline void HfpSmCb::OutgoingIncorrectState ()
{
	uint32 stflag = (HfpSmObj.State_next != HfpSmObj.State) ? DIALAPP_FLAG_NEWSTATE:0;
	CbFunc (DialAppState(HfpSmObj.State), DialAppError_IncorrectState4Call, stflag, 0);
}

inline void HfpSmCb::InCallHeadsetOnOff(bool sound_changed)
{
	uint32 stflag = (HfpSmObj.State_next != HfpSmObj.State) ? DIALAPP_FLAG_NEWSTATE:0;
	if (sound_changed)
		CbFunc (DialAppState(HfpSmObj.State_next), DialAppError_Ok, DIALAPP_FLAG_PCSOUND|stflag, &HfpSmObj.PublicParams);
	else
		CbFunc (DialAppState(HfpSmObj.State_next), DialAppError_Ok, stflag, 0);
}

inline void HfpSmCb::CallEnded()
{
	uint32 stflag = (HfpSmObj.State_next != HfpSmObj.State) ? DIALAPP_FLAG_NEWSTATE:0;
	CbFunc (DialAppState_ServiceConnected, DialAppError_Ok, stflag, 0);
}

inline void HfpSmCb::CallEndedVoiceFailure()
{
	uint32 stflag = (HfpSmObj.State_next != HfpSmObj.State) ? DIALAPP_FLAG_NEWSTATE:0;
	CbFunc (DialAppState_ServiceConnected, DialAppError_ReadWriteScoError, stflag, 0);
}

inline void HfpSmCb::ConnectFailure()
{
	uint32 stflag = (HfpSmObj.State_next != HfpSmObj.State) ? DIALAPP_FLAG_NEWSTATE:0;
	CbFunc (DialAppState_DisconnectedDevicePresent, DialAppError_ConnectFailure, stflag, 0);
}

inline void HfpSmCb::ServiceConnectFailure	()
{
	uint32 stflag = (HfpSmObj.State_next != HfpSmObj.State) ? DIALAPP_FLAG_NEWSTATE:0;
	CbFunc (DialAppState_Connected, DialAppError_ServiceConnectFailure, stflag, 0);
}

inline void HfpSmCb::CallFailure ()
{
	uint32 stflag = (HfpSmObj.State_next != HfpSmObj.State) ? DIALAPP_FLAG_NEWSTATE:0;
	CbFunc (DialAppState_ServiceConnected, DialAppError_CallFailure, stflag, 0);
}

inline void HfpSmCb::Calling ()
{
	uint32 stflag = (HfpSmObj.State_next != HfpSmObj.State) ? DIALAPP_FLAG_NEWSTATE:0;
	CbFunc (DialAppState_Calling, DialAppError_Ok, stflag, 0);
}

inline void HfpSmCb::AbonentInfo ()
{
	uint32 stflag = (HfpSmObj.State_next != HfpSmObj.State) ? DIALAPP_FLAG_NEWSTATE:0;
	CbFunc (DialAppState(HfpSmObj.State), DialAppError_Ok, DIALAPP_FLAG_ABONENT, &HfpSmObj.PublicParams);
}


inline void HfpSmCb::Ring ()
{
	uint32 stflag = (HfpSmObj.State_next != HfpSmObj.State) ? DIALAPP_FLAG_NEWSTATE:0;
	CbFunc (DialAppState_Ringing, DialAppError_Ok, stflag, 0);
}


#endif  // _HFPSM_H_
