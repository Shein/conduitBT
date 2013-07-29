#include "DialApp.h"
#include "DialForm.h"

using namespace System;
using namespace System::Windows::Forms;
using namespace Microsoft::Win32;
using namespace DialAppGui;


#define DIALAPP_REGKEY_HFP_AT_COMMANDS	"HfpAtCommands"


// Error names (should be identical to enum DialAppError)
cchar* DialAppErrorString[] = 
{
	// Common errors
	"Ok",
	"InternalError",
	"InsufficientResources",

	// Init errors
	"InitBluetoothRadioError",
	"InitDriverError",

	// HFP API errors
	"UnknownDevice",
	"IncorrectState4Call",
	"ConnectFailure",
	"ServiceConnectFailure",
	"CallFailure",

	// Audio channel errors
	"OpenScoFailure",
	"ReadWriteScoError",
	"WaveApiError",
};


// State names (should be identical to enum DialAppState)
cchar* DialAppStateString[] = 
{
	"IdleNoDevice",
	"DisconnectedDevicePresent",
	"Connecting",
	"Connected",
	"ServiceConnecting",
	"ServiceConnected",
	"Calling",
	"Ringing",
	"InCallPcSoundOff",
	"InCallPcSoundOn"
};


//C-style callback for DialApp.dll
void DialAppCbFunc (DialAppState state, DialAppError status, DialAppParam* param)
{
	if (status == DialAppError_Ok)
	{
		DialForm::This->SetStateName (%System::String(DialAppStateString[state]));
		switch (state)
		{
			case DialAppState_IdleNoDevice:
				DialForm::This->SetDeviceName ("");
				break;

			case DialAppState_DisconnectedDevicePresent:
				DialForm::This->SetDeviceName (%System::String(param->BthName));
				break;

			case DialAppState_InCallPcSoundOff:
				DialForm::This->SetHeadsetButtonName("PC Sound On");
				goto case_abonent;
			case DialAppState_InCallPcSoundOn:
				DialForm::This->SetHeadsetButtonName("PC Sound Off");
				case_abonent:
				if (param)
					DialForm::This->AddInfoMessage ("Abonent: '" + %System::String(param->Abonent) + "'");
				break;
		}
	}
	else {
		DialForm::This->SetError (%System::String(DialAppStateString[state]), %System::String(DialAppErrorString[status]));
	}
}


void DialForm::DialForm_Load(Object ^sender, EventArgs ^e)
{
	try
	{
		dialappInit	(&DialAppCbFunc);
		wchar *name = dialappGetSelectedDevice();
		chboxAutoServCon->Checked = Registry->ReadInt (DIALAPP_REGKEY_HFP_AT_COMMANDS);
		dialappDebugMode (DialAppDebug_HfpAtCommands, int(chboxAutoServCon->Checked));
	}
	catch (int err)
	{
		tboxLog->AppendText("EXCEPTION: " + %System::String(DialAppErrorString[err]));
	}
}


void DialForm::DialForm_FormClosing(Object ^sender, FormClosingEventArgs ^e)
{
	try
	{
		dialappEnd();
	}
	catch (int err)
	{
		tboxLog->AppendText("EXCEPTION: " + %System::String(DialAppErrorString[err]));
	}
}


void DialForm::chboxAutoServCon_Click(System::Object^ sender, System::EventArgs^ e)
{
	int val = int(chboxAutoServCon->Checked);
	Registry->WriteInt (DIALAPP_REGKEY_HFP_AT_COMMANDS, val);
	dialappDebugMode (DialAppDebug_HfpAtCommands, val);
}

void DialForm::btnClear_Click(Object ^sender, EventArgs ^e)
{
 	tboxLog->Text = "";
}


void DialForm::btnSelectDevice_Click(Object ^sender, EventArgs ^e)
{
	try 
	{
		// The DialForm HWND may be taken from (int*)(this->Handle.ToPointer(), 
		// if needed to be passed to dialappUiSelectDevice
		dialappUiSelectDevice();
	}
	catch (int err)
	{
		tboxLog->AppendText("EXCEPTION: " + %System::String(DialAppErrorString[err]));
	}
}


void DialForm::btnForgetDevice_Click(Object ^sender, EventArgs ^e)
{
	dialappForgetDevice();
}


void DialForm::btnConnect_Click(Object ^sender, EventArgs ^e)
{
	dialappDebugMode (DialAppDebug_ConnectNow);
}


void DialForm::btnDisconnect_Click(Object ^sender, EventArgs ^e)
{
	dialappDebugMode (DialAppDebug_DisconnectNow);
}


void DialForm::btnServCon_Click(Object ^sender, EventArgs ^e)
{
}


void DialForm::btnCall_Click(Object ^sender, EventArgs ^e)
{
	cchar * dialnum = String2Pchar(eboxDialNumber->Text);
	dialappCall(dialnum, chboxAutoHeadset->Checked);
	FreePchar(dialnum);
}


void DialForm::btnAnswer_Click(Object ^sender, EventArgs ^e)
{
	dialappAnswer(chboxAutoHeadset->Checked);
}


void DialForm::btnHeadset_Click(Object ^sender, EventArgs ^e)
{
	dialappPcSound (btnHeadset->Text == "PC Sound On");
}


void DialForm::btnEndCall_Click(Object ^sender, EventArgs ^e)
{
	dialappEndCall();
}



int Main ()
{
	// Enable visual styles and run the form
	Application::EnableVisualStyles();
	Application::Run( gcnew DialAppGui::DialForm() );
	return 0;
}
