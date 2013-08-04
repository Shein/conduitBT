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


void InHand::BeginConnect (uint64 devaddr)
{
	InHandMng::BeginConnect(gcnew BluetoothAddress(devaddr));
}


void InHand::Disconnect()
{
	InHandMng::Disconnect();
}


int InHand::BeginHfpConnect()
{
	return InHandMng::BeginHfpConnect(true);
}


void InHand::StartCall(cchar* dialnumber)
{
	InHandMng::StartCall(%System::String(dialnumber));
}


void InHand::SendDtmf(cchar* dialchar)
{
	InHandMng::SendDtmf(%System::String(dialchar));
}


void InHand::Answer()
{
	InHandMng::Answer();
}


void InHand::EndCall()
{
	InHandMng::EndCall();
}


void InHand::PutOnHold()
{
	InHandMng::PutOnHold();
}


void InHand::ActivateOnHoldCall(int callid)
{
	InHandMng::ActivateHeldCall(callid);
}


void InHand::SendAtCommand( char* at )
{
	String^ clistr = gcnew String(at);
	InHandMng::SendAtCommand(clistr);
}
