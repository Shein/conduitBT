// DialAppCons.cpp : Defines the entry point for the console application.
//

#include <windows.h>
#include <stdio.h>
#include "DialApp.h"


DIALAPP_LINKAGE_VARIABLES;


// Error names (should be identical to enum DialAppError)
cchar* DialAppErrorString[] = 
{
	// Common errors
	"Ok",
	"InternalError",
	"InsufficientResources",
	"DllLoadError",

	// Init errors
	"InitBluetoothRadioError",
	"InitDriverError",
	"InitMediaDeviceError",

	// HFP API errors
	"UnknownDevice",
	"IncorrectState4Call",
	"ConnectFailure",
	"ServiceConnectFailure",
	"CallFailure",

	// Audio channel errors
	"OpenScoFailure",
	"CloseScoFailure",
	"ReadScoError",
	"WriteScoError",
	"WaveApiError",
	"WaveInError",
	"WaveBuffersError",
	"MediaObjectInError",
};



// State names (should be identical to enum DialAppState)
cchar* DialAppStateString[] = 
{
	"Init",
	"IdleNoDevice",
	"DisconnectedDevicePresent",
	"Connecting",
	"Connected",
	"ServiceConnecting",
	"ServiceConnected",
	"Calling",
	"Ringing",
	"InCall"
};


//C-style callback for DialApp.dll
void DialAppCbFunc (DialAppState state, DialAppError status, uint32 flags, DialAppParam* param)
{
	// Show States and Errors
	if (flags & DIALAPP_FLAG_NEWSTATE) {
		if (status == DialAppError_Ok) {
			char *sparam = "";
			if (flags & DIALAPP_FLAG_PCSOUND)
				sparam = (param->PcSound) ? " (PC Sound On)":" (PC Sound Off)";
			printf ("New state %s %s\n", DialAppStateString[state], sparam);
		}
	}
	else {
		if (flags & DIALAPP_FLAG_PCSOUND) {
			// if state was not changed, check whether the current PC Sound state changed (it may be in InCall state)
			printf ("(Current call PC Sound %s\n", (param->PcSound ? "On)":"Off)"));
		}
	}

	// Show Errors
	if (status != DialAppError_Ok)
		printf ("ERROR: %s\n", DialAppErrorString[status]);
}


int main(int argc, char* argv[])
{
	try
	{
		dialappInit(DialAppCbFunc);
		for (;;);
	}
	catch (int err)
	{
		printf ("EXCEPTION: %s\n", DialAppErrorString[err]);
	}

	return 0;
}

