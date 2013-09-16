#include <string.h>
#include "DialApp.h"
#include "DialForm.h"

using namespace System;
using namespace System::Windows::Forms;
using namespace Microsoft::Win32;
using namespace DialAppGui;


//#define DIALAPP_REGKEY_HFP_AT_COMMANDS		"HfpAtCommands"
#define DIALAPP_REGKEY_LAST_DIAL_NUM			"LastDialNumber"
#define DIALAPP_REGKEY_LAST_PC_SOUND_FLAG		"LastPcSoundFlag"

char	 dialappLastDialNumber[50];
bool	 dialappLastPcSoundFlag;
uint64 * dialappDevicesAddresses;


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
			String^ sparam = nullptr;
			if (flags & DIALAPP_FLAG_PCSOUND)
				sparam = (param->PcSound) ? " (PC Sound On)":" (PC Sound Off)";
			DialForm::This->SetStateName (%System::String(DialAppStateString[state]), sparam);
		}
		else {
			DialForm::This->SetError (%System::String(DialAppStateString[state]), %System::String(DialAppErrorString[status]));
		}
	}
	else {
		if (flags & DIALAPP_FLAG_PCSOUND) {
			// if state was not changed, check whether the current PC Sound state changed (it may be in InCall state)
			DialForm::This->AddInfoMessage ("(Current call PC Sound " + (param->PcSound ? "On)":"Off)"));
		}
	}

	// Show Parameters
	if (flags & DIALAPP_FLAG_PCSOUND_PREFERENCE) {
		DialForm::This->SetHeadsetButtonName (param->PcSoundPref);
	}
	if (flags & DIALAPP_FLAG_CURDEV) {
		array<String^>^ items = DialForm::This->InitDevicesCombo();
		DialForm::This->SetDeviceName ((param->CurDevice) ? %System::String(param->CurDevice->Name):"", items);
	}
	if (flags & DIALAPP_FLAG_ABONENT_CURRENT) {
		if (param->AbonentCurrent) {
			DialForm::This->AddInfoMessage ("Abonent Number: '" + %System::String(param->AbonentCurrent->Number) + "'");
			if (param->AbonentCurrent->Name)
				DialForm::This->AddInfoMessage ("Abonent Name: '" + %System::String(param->AbonentCurrent->Name) + "'");
		}
	}
	if (flags & DIALAPP_FLAG_ABONENT_WAITING) {
		if (param->AbonentWaiting)
			DialForm::This->AddInfoMessage ("Waiting Call: '" +  %System::String(param->AbonentWaiting->Name) +" " + %System::String(param->AbonentWaiting->Number) + "'");
		else
			DialForm::This->AddInfoMessage ("Waiting Call: NO CALLS NOW");
	}
	if (flags & DIALAPP_FLAG_ABONENT_HELD) {
		if (param->AbonentHeld)
			DialForm::This->AddInfoMessage ("Call On Hold: '" +  %System::String(param->AbonentHeld->Name) +" " + %System::String(param->AbonentHeld->Number) + "'");
		else
			DialForm::This->AddInfoMessage ("Call On Hold: NO CALLS NOW");
	}
}


array<String^>^  DialForm::InitDevicesCombo()
{
	if (dialappDevicesAddresses)
		delete[] dialappDevicesAddresses;

	DialAppBthDev* devices;
	int n = dialappGetPairedDevices(devices);
	dialappDevicesAddresses = new uint64[n];
	array<String^>^ items = gcnew array<String^>(n+1);
	for (int i=0; i<n; i++)	{
		items[i] = gcnew String(devices[i].Name);
		dialappDevicesAddresses[i] = devices[i].Address;
	}
	items[n] = "";
	return items;
}


void DialForm::DialForm_Load(Object ^sender, EventArgs ^e)
{
	try
	{
		eboxDialNumber->Text = Registry->Read(DIALAPP_REGKEY_LAST_DIAL_NUM);
		cchar * dialnum = String2Pchar(eboxDialNumber->Text);
		strncpy (dialappLastDialNumber,dialnum,sizeof(dialappLastDialNumber)-1);
		FreePchar(dialnum);

		dialappLastPcSoundFlag = (bool) Registry->ReadInt(DIALAPP_REGKEY_LAST_PC_SOUND_FLAG);
		btnHeadset->Text = (dialappLastPcSoundFlag) ? "PC Sound Off":"PC Sound On";

		//chboxAutoServCon->Checked = Registry->ReadInt (DIALAPP_REGKEY_HFP_AT_COMMANDS);
		//dialappDebugMode (DialAppDebug_HfpAtCommands, int(chboxAutoServCon->Checked));

		dialappInit	(&DialAppCbFunc, dialappLastPcSoundFlag);
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
		cchar * dialnum = String2Pchar(eboxDialNumber->Text);
		if (strcmp(dialappLastDialNumber, dialnum)!=0) {
			strncpy (dialappLastDialNumber,dialnum,sizeof(dialappLastDialNumber)-1);
			Registry->Write (DIALAPP_REGKEY_LAST_DIAL_NUM, eboxDialNumber->Text);
		}

		bool soundflag = (btnHeadset->Text == "PC Sound Off");
		if (dialappLastPcSoundFlag != soundflag)
			Registry->WriteInt(DIALAPP_REGKEY_LAST_PC_SOUND_FLAG, int(soundflag));

		dialappEnd();
	}
	catch (int err)
	{
		tboxLog->AppendText("EXCEPTION: " + %System::String(DialAppErrorString[err]));
	}
}


void DialForm::chboxAutoServCon_Click(System::Object^ sender, System::EventArgs^ e)
{
// 	int val = int(chboxAutoServCon->Checked);
// 	Registry->WriteInt (DIALAPP_REGKEY_HFP_AT_COMMANDS, val);
// 	dialappDebugMode (DialAppDebug_HfpAtCommands, val);
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
		dialappSelectDevice();
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


void DialForm::eboxDevice_Click(Object ^sender, EventArgs ^e)
{
	if (eboxDevice->Text == eboxDeviceText)
		return;

	if (eboxDevice->Text == "") {
		eboxDevice->Text = eboxDeviceText;	// ForgetDevice must be used instead
		return;
	}

	for (int i=0; i<eboxDevice->Items->Count; i++) {
		if (eboxDevice->Text->Equals(eboxDevice->Items[i]->ToString())) {
			dialappSelectDevice (dialappDevicesAddresses[i]);
			return;
		}
	}

	// Strange error happened
	eboxDevice->Text = eboxDeviceText;
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
	dialappCall(dialnum);
	FreePchar(dialnum);
}


void DialForm::btnAnswer_Click(Object ^sender, EventArgs ^e)
{
	dialappAnswer();
}


void DialForm::btnPcSound_Click(Object ^sender, EventArgs ^e)
{
	dialappPcSound (btnHeadset->Text == "PC Sound On");
}


void DialForm::btnEndCall_Click(Object ^sender, EventArgs ^e)
{
	dialappEndCall();
}


void DialForm::btn1_Click(Object ^sender, EventArgs ^e)
{
	dialappSendDtmf('1');
}

void DialForm::btn2_Click(Object ^sender, EventArgs ^e)
{
	dialappSendDtmf('2');
}

void DialForm::btn3_Click(Object ^sender, EventArgs ^e)
{
	dialappSendDtmf('3');
}

void DialForm::btn4_Click(Object ^sender, EventArgs ^e)
{
	dialappSendDtmf('4');
}

void DialForm::btn5_Click(Object ^sender, EventArgs ^e)
{
	dialappSendDtmf('5');
}

void DialForm::btn6_Click(Object ^sender, EventArgs ^e)
{
	dialappSendDtmf('6');
}

void DialForm::btn7_Click(Object ^sender, EventArgs ^e)
{
	dialappSendDtmf('7');
}

void DialForm::btn8_Click(Object ^sender, EventArgs ^e)
{
	dialappSendDtmf('8');
}

void DialForm::btn9_Click(Object ^sender, EventArgs ^e)
{
	dialappSendDtmf('9');
}

void DialForm::button_diez_Click(Object ^sender, EventArgs ^e)
{
	dialappSendDtmf('#');
}

void DialForm::button_zero_Click(Object ^sender, EventArgs ^e)
{
	dialappSendDtmf('0');
}
void DialForm::button_star_Click(Object ^sender, EventArgs ^e)
{
	dialappSendDtmf('*');
}

void DialForm::button_SendAT_Click(Object ^sender, EventArgs ^e)
{
	//char* str = (char*)(void*)Marshal::StringToHGlobalAnsi(textBox_AT_command->Text);
}

void DialForm::button_Hold_Click(Object ^sender, EventArgs ^e)
{
	dialappPutOnHold();
}


int Main ()
{
	// Enable visual styles and run the form
	Application::EnableVisualStyles();
	Application::Run( gcnew DialAppGui::DialForm() );
	return 0;
}
