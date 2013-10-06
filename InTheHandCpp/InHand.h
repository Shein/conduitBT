/*******************************************************************\
 Filename    :  InHand.h
 Purpose     :  InTheHand.NET C++ unmanaged class wrapper
\*******************************************************************/

#pragma once
#pragma managed(push, off)


#include "def.h"
#include "deblog.h"
#include "DialAppType.h"


extern DebLog InHandLog;

class HfpIndicators;


/*
 ****************************************************************************************
 C++ wrapper for InTheHand .NET library for Bluetooth devices.
 It is fully static class controlled by HFP State Machine (HfpSm).
 ****************************************************************************************
 */
class InHand
{
  public:
	static void Init ();
	static void End  ();

	static int	GetDevices (DialAppBthDev* &devices);
	static void RescanDevices ();
	static DialAppBthDev* FindDevice (uint64 address, bool rescan = false);

	static void ClearIndicatorsNumbers();
	static void SetIndicatorsNumbers(int call, int callsetup, int callheld);

	static void BeginConnect	(uint64 devaddr);
	static int  BeginHfpConnect	();
	static void Disconnect		();
	static void StartCall		(cchar* dialnumber);
	static void SendDtmf		(cchar* dialchar);
	static void Answer			();
	static void EndCall			();
	static void PutOnHold		();
	static void ListCurrentCalls();

  public:
	static DialAppBthDev  *Devices;
	static int			   NumDevices;
	static HfpIndicators  *CurIndicators;
};



#pragma managed(pop)

