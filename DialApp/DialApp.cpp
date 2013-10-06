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

static uint64			 dialappCurStroredAddr;
static HfpSmInitReturn*  dialappInitEvent;


static uint64 dialappRestoreDevAddr()
{
	uint64 val;
	DWORD  size = sizeof(val);
	int ret = RegGetValue(HKEY_CURRENT_USER, DIALAPP_REGPATH, DIALAPP_REGKEY_BTHDEV, RRF_RT_ANY, 0, (void*)&val, &size);
	if (ret != ERROR_SUCCESS)
		return 0;
	return (dialappCurStroredAddr = val);
}


static bool dialappStoreDevAddr (uint64 val)
{
	if (dialappCurStroredAddr == val)
		return true;

	HKEY hkey;
	int ret = RegCreateKey(HKEY_CURRENT_USER, DIALAPP_REGPATH, &hkey);
	if (ret == ERROR_SUCCESS) {
		ret = RegSetValueEx(hkey, DIALAPP_REGKEY_BTHDEV, 0, REG_QWORD, (BYTE*)&val, sizeof(val));
		RegCloseKey(hkey);
		dialappCurStroredAddr = val;
	}
	if (ret != ERROR_SUCCESS) 
		LogMsg ("dialappStoreDevAddr: Registry Access Error");
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
		LogMsg ("BluetoothSelectDevices returned TRUE");
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

	LogMsg ("BluetoothSelectDevices returned FALSE, GetLastError = %d",  GetLastError());
	return 0;
}


/***********************************************************************************************\
										Callback function
\***********************************************************************************************/

static DialAppCb dialappUserCb;

static void dialappCb (DialAppState state, DialAppError status, uint32 flags, DialAppParam* param)
{
	if (status == DialAppError_Ok)
	{
		if (((flags & DIALAPP_FLAG_INITSTATE) == 0) && (flags & DIALAPP_FLAG_CURDEV)) {
			dialappStoreDevAddr ((param->CurDevice) ? param->CurDevice->Address : 0);
		}
	}

	LogMsg ("dialappUserCb: state=%d, status=%d, flags=%X", state, status, flags);
	dialappUserCb(state, status, flags, param);
}



/***********************************************************************************************\
										Public functions
\***********************************************************************************************/

void dialappInit (DialAppCb cb, bool pcsound)
{
	DebLog::Init("DialApp");

	LogMsg ("dialappInit: starting DialApp");
	dialappUserCb = cb;

	Timer::Init();
	InHand::Init();
	SmBase::Init();
	ScoApp::Init();

	// Because of some errors from HFP SM may be thrown in separate threads during init, 
	// we need to implement here the mechanism to catch such asynchronous errors.
	dialappInitEvent = new HfpSmInitReturn();
	HfpSm::Init(dialappCb, dialappInitEvent);
	dialappInitEvent->SignalEvent.Wait();
	int err = dialappInitEvent->RetCode;
	delete dialappInitEvent;
	dialappInitEvent = 0;

	if (err)
		throw err;	// if the error exists it was already printed to the debug console

	// At init phase we can directly access SM's parameters
	HfpSmObj.PublicParams.PcSoundPref = pcsound;

	uint64 addr = dialappRestoreDevAddr();
	if (InHand::FindDevice(addr))
		HfpSm::PutEvent_SelectDevice(addr);
}


void dialappEnd ()
{
	LogMsg ("dialappEnd: stopping DialApp");
	// TODO gracefully finalize the SM
	// HfpSm::PutEvent_Disconnect();
	HfpSm::End();
	ScoApp::End();
	SmBase::End();
	InHand::End();
	Timer::End();
	DebLog::End();
}


int dialappGetPairedDevices (DialAppBthDev* &devices)
{
	return InHand::GetDevices(devices);
}


DialAppBthDev* dialappGetSelectedDevice ()

{
	return HfpSmObj.PublicParams.CurDevice;
}


int	dialappGetCurrentState ()
{
	return HfpSmObj.State;	// Should be identical to DialAppState
}


void dialappSelectDevice (uint64 devaddr)
{
	if (!devaddr) {
		devaddr = dialappBluetoothSelectDevice ((HWND)0);
		// ***************************************************************************************
		// Windows 8 workaround: if dialappBluetoothSelectDevice returned 0 we still try to detect the win8 
		// bug when user completed this dialog successfully but for some reason BluetoothSelectDevices failed.
		if (!devaddr) {
			int i, j, n = InHand::NumDevices;
			uint64 * addr = new uint64[n];
			for (i=0; i<n; i++)
				addr[i] = InHand::Devices[i].Address;
			InHand::RescanDevices();
			if (n + 1 == InHand::NumDevices) {
				for (i=0; i<InHand::NumDevices; i++) {
					for (j=0; j<n; j++)
						if (InHand::Devices[i].Address == addr[j])
							goto next_i;
					// InHand::Devices[i] is the new device!
					devaddr = InHand::Devices[i].Address;
					break;
					next_i:;
				}
			}
			delete[] addr;
		}
		// End of Windows 8 workaround
		// ***************************************************************************************
	}
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


void dialappCall (cchar* dialnumber)
{
	HfpSm::PutEvent_StartOutgoingCall(dialnumber);
}


void dialappAnswer ()
{
	HfpSm::PutEvent_Answer();
}


void dialappSendDtmf (cchar dialchar)
{
	HfpSm::PutEvent_SendDtmf(dialchar);
}


void dialappPutOnHold()
{
	HfpSm::PutEvent_PutOnHold();
}


void dialappEndCall ()
{
	HfpSm::PutEvent_CallEnd();
}


void dialappPcSound (bool pcsound)
{
	HfpSm::PutEvent_Headset(pcsound);
}


void dialappDebugMode (DialAppDebug debugtype, int mode)
{
	switch (debugtype)
	{
		case DialAppDebug_ConnectNow:
			if (HfpSmObj.PublicParams.CurDevice)
				HfpSm::PutEvent_ConnectStart(HfpSmObj.PublicParams.CurDevice->Address);
			break;

		case DialAppDebug_DisconnectNow:
			HfpSm::PutEvent_Disconnect();
			break;
	}
}

