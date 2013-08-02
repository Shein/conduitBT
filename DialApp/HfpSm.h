/*******************************************************************\
 Filename    :  HfpSm.h
 Purpose     :  Main HFP State Machine
\*******************************************************************/

#ifndef _HFPSM_H_
#define _HFPSM_H_

#include "DialAppType.h"
#include "InHandType.h"
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
	void DevicePresent			(InHandDev * dev);
	void DeviceForgot			();
	void HfpConnected			();
	void OutgoingIncorrectState	();
	void Calling				();
	void InCallHeadsetOn		();
	void InCallHeadsetOff		();
	void CallEnded				();
	void ConnectFailure			();
	void ServiceConnectFailure	();
	void CallFailure			();
	void CallEndedVoiceFailure	();
	void AbonentInfo			(wchar * abonent);
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
	HfpSm(): SMT<HfpSm>("HfpSm  "), MyTimer(SM_HFP, SMEV_Timeout), ScoAppObj(0), CurDevice(0) {};

	void Construct();
	void Destruct();

  public:
	SmTimer		MyTimer;
	ScoApp*		ScoAppObj;
	InHandDev*	CurDevice;
	bool		HeadsetOn;
	int			AtResponsesCnt;
	int			AtResponsesNum;

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

	static void PutEvent_Disconnected ()
	{
		SMEVENT Event = {SM_HFP, SMEV_Disconnected};
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

	static void PutEvent_HeadsetOn ()
	{
		SMEVENT Event = {SM_HFP, SMEV_HeadsetOn};
		SmBase::PutEvent (&Event, SMQ_HIGH);
	}

	static void PutEvent_HeadsetOff ()
	{
		SMEVENT Event = {SM_HFP, SMEV_HeadsetOff};
		SmBase::PutEvent (&Event, SMQ_LOW);
	}

	static void PutEvent_StartOutgoingCall (cchar* dialnumber, bool headset)
	{
		SMEVENT Event = {SM_HFP, SMEV_StartOutgoingCall};
		Event.Param.CallNumber = new (dialnumber) CallInfo<cchar>(dialnumber);
		Event.Param.HeadsetOn  = headset;
		SmBase::PutEvent (&Event, SMQ_LOW);
	}

	static void PutEvent_Answer (bool headset = false)
	{
		SMEVENT Event = {SM_HFP, SMEV_Answer};
		Event.Param.HeadsetOn = headset;
		SmBase::PutEvent (&Event, SMQ_LOW);
	}

	static void PutEvent_AtResponse (SMEV_ATRESPONSE resp)
	{
		SMEVENT Event = {SM_HFP, SMEV_AtResponse};
		Event.Param.AtResponse = resp;
		SmBase::PutEvent (&Event, SMQ_HIGH);
	}

	static void PutEvent_CallIdentity (wchar* dialinfo)
	{
		SMEVENT Event = {SM_HFP, SMEV_AtResponse};
		Event.Param.AtResponse	= SMEV_AtResponse_CallIdentity;
		Event.Param.Abonent		= new (dialinfo) CallInfo<wchar>(dialinfo);
		SmBase::PutEvent (&Event, SMQ_LOW);
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
		Event.Param.dtmf = dialinfo;
		SmBase::PutEvent (&Event, SMQ_LOW);
	}


  // Help functions
  private:
	void StartVoice	();
	void StopVoice	();

  // Transitions
  private:
	bool ForgetDevice		  (SMEVENT* ev, int param);
	bool SelectDevice		  (SMEVENT* ev, int param);
	bool Disconnected		  (SMEVENT* ev, int param);
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

  // Choices
  private:
	int  GetVoiceState1		  (SMEVENT* ev);
	int  GetVoiceState2		  (SMEVENT* ev);
};


extern HfpSm HfpSmObj;


inline void HfpSmCb::DevicePresent (InHandDev * dev)
{
	DialAppParam p = { dev->Address, dev->Name };
	CbFunc (DialAppState_DisconnectedDevicePresent, DialAppError_Ok, &p);
}

inline void HfpSmCb::DeviceForgot ()
{
	CbFunc (DialAppState_IdleNoDevice, DialAppError_Ok, 0);
}

inline void HfpSmCb::HfpConnected ()
{
	CbFunc (DialAppState_ServiceConnected, DialAppError_Ok, 0);
}

inline void HfpSmCb::OutgoingIncorrectState ()
{
	CbFunc (DialAppState(HfpSmObj.State), DialAppError_IncorrectState4Call, 0);
}

inline void HfpSmCb::InCallHeadsetOn()
{
	CbFunc (DialAppState_InCallPcSoundOn, DialAppError_Ok, 0);
}

inline void HfpSmCb::InCallHeadsetOff()
{
	CbFunc (DialAppState_InCallPcSoundOff, DialAppError_Ok, 0);
}

inline void HfpSmCb::CallEnded()
{
	CbFunc (DialAppState_ServiceConnected, DialAppError_Ok, 0);
}

inline void HfpSmCb::CallEndedVoiceFailure()
{
	CbFunc (DialAppState_ServiceConnected, DialAppError_ReadWriteScoError, 0);
}

inline void HfpSmCb::ConnectFailure()
{
	CbFunc (DialAppState_DisconnectedDevicePresent, DialAppError_ConnectFailure, 0);
}

inline void HfpSmCb::ServiceConnectFailure	()
{
	CbFunc (DialAppState_Connected, DialAppError_ServiceConnectFailure, 0);
}

inline void HfpSmCb::CallFailure ()
{
	CbFunc (DialAppState_ServiceConnected, DialAppError_CallFailure, 0);
}

inline void HfpSmCb::Calling ()
{
	CbFunc (DialAppState_Calling, DialAppError_Ok, 0);
}

inline void HfpSmCb::AbonentInfo (wchar * abonent)
{
	DialAppParam p;
	p.Abonent = abonent;
	CbFunc (DialAppState(HfpSmObj.State), DialAppError_Ok, &p);
}


inline void HfpSmCb::Ring ()
{
	CbFunc (DialAppState_Ringing, DialAppError_Ok, 0);
}


#endif  // _HFPSM_H_
