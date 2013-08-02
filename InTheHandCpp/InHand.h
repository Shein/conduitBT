#pragma once
#pragma managed(push, off)


#include "def.h"
#include "deblog.h"
#include "DialAppType.h"
#include "InHandType.h"


extern DebLog InHandLog;


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

	static InHandDev* FindDevice (uint64 address, bool rescan = false);

	static void BeginConnect	(uint64 devaddr);
	static int  BeginHfpConnect	();
	static void Disconnect		();
	static void StartCall		(cchar* dialnumber);
	static void SendDtmf		(cchar* dialchar);
	static void Answer			();
	static void EndCall			();

  public:
	static InHandDev  *Devices;
	static int		   NumDevices;
	static bool		   EnableHfpAtCommands;
};


#pragma managed(pop)
