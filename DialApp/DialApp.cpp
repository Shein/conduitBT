/********************************************************************************************\
 Library     :  HFP DialApp (Bluetooth HFP library)
 Filename    :  DialApp.cpp
 Purpose     :  Defines the exported functions for the DialApp.dll
\********************************************************************************************/

#include <windows.h>
#include <BluetoothAPIs.h>

#include "DialApp.h"
#include "deblog.h"
#include "timer.h"
#include "InHand.h"
#include "ScoApp.h"
#include "smBase.h"
#include "HfpSm.h"
#include "CallInfo.h"


// Registry path for keeping HFP DialApp parameters
#define DIALAPP_REGPATH			"SOFTWARE\\Conduit\\HfpDialApp"
#define DIALAPP_REGKEY_BTHDEV	"CurrentDeviceAddress"


/***********************************************************************************************\
										Static functions
\***********************************************************************************************/

static uint64 dialappRestoreDevAddr()
{
	uint64 val;
	DWORD  size = sizeof(val);
	int ret = RegGetValue(HKEY_LOCAL_MACHINE, DIALAPP_REGPATH, DIALAPP_REGKEY_BTHDEV, RRF_RT_ANY, 0, (void*)&val, &size);
	return (ret == ERROR_SUCCESS) ? val : 0;
}

static bool dialappStoreDevAddr (uint64 val)
{
	HKEY hkey;
	int ret = RegCreateKey(HKEY_LOCAL_MACHINE, DIALAPP_REGPATH, &hkey);
	if (ret == ERROR_SUCCESS) {
		ret = RegSetValueEx(hkey, DIALAPP_REGKEY_BTHDEV, 0, RRF_RT_REG_QWORD, (BYTE*)&val, sizeof(val));
		RegCloseKey(hkey);
	}
	return (ret == ERROR_SUCCESS);
}


static uint64 dialappBluetoothSelectDevice (HWND hwnd)
{
	BLUETOOTH_SELECT_DEVICE_PARAMS btsdp = { sizeof(btsdp) };

	btsdp.cNumOfClasses = 0; // search for all devices
	btsdp.prgClassOfDevices = 0;
	btsdp.pszInfo = L"Select Device..";
	btsdp.hwndParent = hwnd;
	btsdp.fForceAuthentication = FALSE;
	btsdp.fShowAuthenticated = TRUE;
	btsdp.fShowRemembered = TRUE;
	btsdp.fShowUnknown = TRUE;
	btsdp.fAddNewDeviceWizard = TRUE;
	btsdp.fSkipServicesPage = TRUE;
	btsdp.pfnDeviceCallback = 0; // no callback
	btsdp.pvParam = 0;
	btsdp.cNumDevices = 0; // no limit
	btsdp.pDevices = 0;

	if (BluetoothSelectDevices(&btsdp))
	{
		uint64 addr;
		BLUETOOTH_DEVICE_INFO * pbtdi = btsdp.pDevices;
		for (unsigned cDevice = 0; cDevice < btsdp.cNumDevices; cDevice ++)
		{
			if (pbtdi->fRemembered) {
				addr = pbtdi->Address.ullLong;
				break;
			}
			pbtdi = (BLUETOOTH_DEVICE_INFO*) ((BYTE*)pbtdi + pbtdi->dwSize);
		}

		BluetoothSelectDevicesFree(&btsdp);
		return addr;
	}
	return 0;
}

/***********************************************************************************************\
										Callback function
\***********************************************************************************************/

static DialAppCb dialappUserCb;

static void dialappCb (DialAppState state, DialAppError status, DialAppParam* param)
{
	if (status == DialAppError_Ok)
	{
		switch (state)
		{
			case DialAppState_DisconnectedDevicePresent:
				dialappStoreDevAddr (param->BthAddr);
				break;

			case DialAppState_IdleNoDevice:
				dialappStoreDevAddr (0);
				break;
		}
	}

	dialappUserCb(state, status, param);
}



/***********************************************************************************************\
										Public functions
\***********************************************************************************************/

void dialappInit (DialAppCb cb)
{
	dialappUserCb = cb;

	DebLog::Init("DialApp");
	Timer::Init();
	InHand::Init();
	SmBase::Init();
	ScoApp::Init();
	HfpSm::Init(dialappCb);

	uint64 addr = dialappRestoreDevAddr();
	if (InHand::FindDevice(addr))
		HfpSm::PutEvent_SelectDevice(addr);
}


void dialappEnd ()
{
	HfpSm::End();
	ScoApp::End();
	SmBase::End();
	InHand::End();
	Timer::End();
	DebLog::End();
}


wchar* dialappGetSelectedDevice ()
{
	return (HfpSmObj.CurDevice) ? HfpSmObj.CurDevice->Name : 0;
}


int	dialappGetCurrentState ()
{
	return HfpSmObj.State;	// Should be identical to DialAppState
}


void dialappUiSelectDevice ()
{
	uint64 devaddr = dialappBluetoothSelectDevice ((HWND)0);

	if (devaddr) {
		// The second parameter to FindDeviceIndex rescan=true in order to update Devices array;
		// the new selected by user device should be found in this updated list
		if (!InHand::FindDevice (devaddr,true))
			throw DialAppError_UnknownDevice;
		HfpSm::PutEvent_SelectDevice(devaddr);
	}
}


void dialappForgetDevice ()
{
	HfpSm::PutEvent_ForgetDevice();
}


void dialappCall (cchar* dialnumber, bool pcsound)
{
	HfpSm::PutEvent_StartOutgoingCall(dialnumber, pcsound);
}


void dialappAnswer (bool pcsound)
{
		HfpSm::PutEvent_Answer(pcsound);		
}

void dialappSendDtmf(cchar dialchar)
{
	HfpSm::PutEvent_SendDtmf(dialchar);
}

void dialappPutOnHold()
{
	HfpSm::PutEvent_PutOnHold();
}


void dialappActivateHeldCall(int callid)
{

}

void dialappEndCall (int callID)
{
	HfpSm::PutEvent_EndCall();
}

void dialappSendAT(char* at)
{
	InHand::SendAtCommand(at);
}


void dialappPcSound (bool pcsound)
{
	if (pcsound)
		HfpSm::PutEvent_HeadsetOn();
	else
		HfpSm::PutEvent_HeadsetOff();
}


void dialappDebugMode (DialAppDebug debugtype, int mode)
{
	switch (debugtype)
	{
		case DialAppDebug_HfpAtCommands:
			InHand::EnableHfpAtCommands = (bool) mode;
			break;

		case DialAppDebug_ConnectNow:
			if (HfpSmObj.CurDevice)
				HfpSm::PutEvent_ConnectStart(HfpSmObj.CurDevice->Address);
			break;

		case DialAppDebug_DisconnectNow:
			InHand::Disconnect ();
			break;
	}
}

