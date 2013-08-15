#include "InHand.h"
#include "InHandMng.h"


DebLog InHandLog("InHand ");


/***********************************************************************************************\
										Static data
\***********************************************************************************************/

DialAppBthDev  *InHand::Devices;
int				InHand::NumDevices;



/***********************************************************************************************\
									Public Static functions
\***********************************************************************************************/

void InHand::Init ()
{
	InHandMng::Init();
	NumDevices = GetDevices(Devices);
}


void InHand::End ()
{
	InHandMng::FreeDevices(Devices,NumDevices);
	InHandMng::End();
}


int	InHand::GetDevices (DialAppBthDev* &devices)
{
	return InHandMng::GetDevices(devices);
}


DialAppBthDev* InHand::FindDevice (uint64 address, bool rescan)
{
	if (rescan) {
		InHandMng::FreeDevices(Devices,NumDevices);
		NumDevices = InHandMng::GetDevices(Devices);
	}

	for (int i=0; i<NumDevices; i++) {
		if (Devices[i].Address == address)
			return &Devices[i];
	}
	return 0;
}


void InHand::ClearIndicatorsNumbers ()
{
	InHandLog.LogMsg("ClearIndicatorsNumbers");
	InHandMng::ClearIndicatorsNumbers ();
}


void InHand::SetIndicatorsNumbers (int call, int callsetup, int callheld)
{
	InHandLog.LogMsg("SetIndicatorsNumbers: call=%d, callsetup=%d, callheld=%d", call, callsetup, callheld);
	InHandMng::SetIndicatorsNumbers (call, callsetup, callheld);
}


void InHand::BeginConnect (uint64 devaddr)
{
	InHandLog.LogMsg("About to BeginConnect");
	InHandMng::BeginConnect(gcnew BluetoothAddress(devaddr));
}


void InHand::Disconnect()
{
	InHandLog.LogMsg("About to Disconnect");
	InHandMng::Disconnect();
}


int InHand::BeginHfpConnect()
{
	InHandLog.LogMsg("About to BeginHfpConnect");
	return InHandMng::BeginHfpConnect();
}


void InHand::StartCall(cchar* dialnumber)
{
	InHandLog.LogMsg("About to StartCall");
	InHandMng::StartCall(%System::String(dialnumber));
}


void InHand::SendDtmf(cchar* dialchar)
{
	InHandMng::SendDtmf(%System::String(dialchar));
}


void InHand::Answer()
{
	InHandLog.LogMsg("About to Answer");
	InHandMng::Answer();
}


void InHand::EndCall()
{
	InHandLog.LogMsg("About to EndCall");
	InHandMng::EndCall();
}


void InHand::PutOnHold()
{
	InHandLog.LogMsg("About to PutOnHold");
	InHandMng::PutOnHold();
}


void InHand::SendAtCommand( char* at )
{
	String^ clistr = gcnew String(at);
	InHandMng::SendAtCommand(clistr);
}


void InHand::ListCurrentCalls()
{
	InHandMng::ListCurrentCalls();
}

