#define INITGUID

#pragma managed(push, off)

#include "def.h"
#include "ScoApp.h"
#include "hfppublic.h"
#include "DialAppType.h"



/***********************************************************************************************\
										Static data
\***********************************************************************************************/

PSP_DEVICE_INTERFACE_DETAIL_DATA	ScoApp::DeviceInterfaceDetailData;



/***********************************************************************************************\
									Public Static functions
\***********************************************************************************************/

void ScoApp::Init ()
{
    HDEVINFO					HardwareDeviceInfo;
    SP_DEVICE_INTERFACE_DATA	DeviceInterfaceData;
    ULONG						len, reqlen = 0;
    BOOL						bres;

    HardwareDeviceInfo = SetupDiGetClassDevs (&HFP_DEVICE_INTERFACE, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (HardwareDeviceInfo == INVALID_HANDLE_VALUE)
		throw ::IntException (DialAppError_InitDriverError, "SetupDiGetClassDevs failed!");

    DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    bres = SetupDiEnumDeviceInterfaces(HardwareDeviceInfo, 0, &HFP_DEVICE_INTERFACE, 0, &DeviceInterfaceData);
    if (!bres) 	{
        SetupDiDestroyDeviceInfoList (HardwareDeviceInfo);
		throw ::IntException (DialAppError_InitDriverError, "SetupDiEnumDeviceInterfaces failed");
    }

    SetupDiGetDeviceInterfaceDetail (HardwareDeviceInfo, &DeviceInterfaceData, 0, 0, &reqlen, 0);
    DeviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) LocalAlloc(LMEM_FIXED, reqlen);
    if (!DeviceInterfaceDetailData) {
        SetupDiDestroyDeviceInfoList (HardwareDeviceInfo);
		throw ::IntException (DialAppError_InitDriverError, "Failed to allocate memory for the device interface detail data");
    }

    DeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
    len = reqlen;

    bres = SetupDiGetDeviceInterfaceDetail (HardwareDeviceInfo, &DeviceInterfaceData, DeviceInterfaceDetailData, len, &reqlen, 0);
    if (!bres) {
        SetupDiDestroyDeviceInfoList(HardwareDeviceInfo);       
        LocalFree(DeviceInterfaceDetailData);
        DeviceInterfaceDetailData = 0;
		throw ::IntException (DialAppError_InitDriverError, "Error in SetupDiGetDeviceInterfaceDetail");
    }

    SetupDiDestroyDeviceInfoList(HardwareDeviceInfo);
    ::LogMsg("ScoApp::Init finished");
}


void ScoApp::End ()
{
	if (DeviceInterfaceDetailData) {
		LocalFree(DeviceInterfaceDetailData);
		DeviceInterfaceDetailData = 0;
	}
}



/***********************************************************************************************\
							Private & protected ScoApp methods
\***********************************************************************************************/


void ScoApp::OpenDriver()
{
    hDevice = CreateFile(DeviceInterfaceDetailData->DevicePath,
                         GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         0,
                         OPEN_EXISTING,
                         FILE_FLAG_OVERLAPPED,
                         0);

    if (hDevice == INVALID_HANDLE_VALUE) {
		hDevice = 0;
		throw IntException (DialAppError_InitDriverError, "Failed to open device, GetLastError %d", GetLastError());
	}
}


void ScoApp::ReopenDriver ()
{
	CloseHandle(hDevice);
	OpenDriver();
}


// virtual from Thread
void ScoApp::Run()
{
	enum { NumEvents = 2 };
	HANDLE    waithandles[NumEvents] = { HANDLE(EventScoConnect.GetWaitHandle()), HANDLE(EventScoDisconnect.GetWaitHandle()) };
	ScoAppCb  callbacks  [NumEvents] = { ConnectCb, DisconnectCb };
 
 	EventScoConnect.Reset();
	EventScoDisconnect.Reset();

	LogMsg("Task started...");

	for (;;)
	{
		DWORD ret = WaitForMultipleObjects (NumEvents, waithandles, FALSE, INFINITE);
		if (Destructing)
			break;
		switch (ret)
		{
			case WAIT_OBJECT_0:
			case WAIT_OBJECT_0 + 1:
				callbacks [ret - WAIT_OBJECT_0] ();
				break;
			default:
				LogMsg("WaitForMultipleObjects returned %X", ret);
		}
	}
}


/***********************************************************************************************\
									Public ScoApp methods
\***********************************************************************************************/

void ScoApp::Construct (ScoAppCb connect_cb, ScoAppCb disconnect_cb)
{
    LogMsg("HFP Device path: %s", DeviceInterfaceDetailData->DevicePath);

    DestAddr	 = 0;
	Open		 = false;
	Destructing  = false;
	ConnectCb	 = connect_cb;
	DisconnectCb = disconnect_cb;

	OpenDriver();

	EventScoConnect.Reset();
	EventScoDisconnect.Reset();

	Thread::Construct();
	Thread::Execute();

	LogMsg("Thread ID = %d", Thread::GetThreadId());

	WaveOutDev = new WaveOut(this);
	WaveInDev  = new WaveIn(this);
}


void ScoApp::Destruct()
{
    if (hDevice && hDevice!=INVALID_HANDLE_VALUE)
		CloseHandle(hDevice);
	hDevice = 0;

	// Call Destruct method instead of delete! Actually it deletes. See remarks at Destruct()
	WaveOutDev->Destruct();
	WaveInDev->Destruct();

	Destructing = true;
	EventScoConnect.Signal();	// no matter which event to signal
	Thread::WaitEnding();
	LogMsg("Destructed");
}


void ScoApp::StartServer (uint64 destaddr, bool readiness)
{
	LogMsg("About to Start SCO server");

	if (!destaddr)
		throw IntException (DialAppError_InternalError, "Destination address cannot be null");

	HFP_REG_SERVER params = { destaddr, HANDLE(EventScoConnect.GetWaitHandle()), HANDLE(EventScoDisconnect.GetWaitHandle()), BOOL(readiness) };
	unsigned long nbytes;
    if (!DeviceIoControl (hDevice, IOCTL_HFP_REG_SERVER, &params, sizeof(params), 0, 0, &nbytes, 0))
		throw IntException (DialAppError_OpenScoFailure, "Register SCO Server FAILED, GetLastError %d", GetLastError());

	DestAddr = destaddr;
}


void ScoApp::StopServer ()
{
	if (IsStarted()) {
		LogMsg("About to Stop SCO server");
		unsigned long nbytes;
		if (!DeviceIoControl (hDevice, IOCTL_HFP_UNREG_SERVER, 0, 0, 0, 0, &nbytes, 0)) {
			// Do not throw exceptions on stop server
			LogMsg ("Unregister SCO Server FAILED, GetLastError %d", GetLastError());
		}

		DestAddr = 0;
	}
}


void ScoApp::OpenSco (bool waveonly)
{
	LogMsg("About to Open SCO channel (waveonly=%d)", waveonly);

	if (!IsConstructed())
		throw IntException (DialAppError_InternalError, "ScoApp::Construct was not called");

	if (IsOpen())
		throw IntException (DialAppError_InternalError, "ScoApp::CloseSco was not called");

	if (!waveonly) {
		unsigned long nbytes;
		if (!DeviceIoControl (hDevice, IOCTL_HFP_OPEN_SCO, 0, 0, 0, 0, &nbytes, 0))
			throw IntException (DialAppError_OpenScoFailure, "Failed to open SCO, GetLastError %d", GetLastError());
	}

	Open = true;
}


void ScoApp::CloseSco (bool waveonly)
{
	if (IsOpen()) {
		LogMsg("About to Close SCO channel (waveonly=%d)", waveonly);
		WaveOutDev->Stop();
		WaveInDev->Stop();
		if (!waveonly)
			ReopenDriver();
		Open = false;
		DestAddr = 0;
	}
}

void ScoApp::CloseScoLowLevel ()
{
	unsigned long nbytes;
	DeviceIoControl (hDevice, IOCTL_HFP_CLOSE_SCO, 0, 0, 0, 0, &nbytes, 0);
}

void ScoApp::VoiceStart ()
{
	LogMsg("About to start passing voice");
	WaveOutDev->Play();
	WaveInDev->Play();
}


void ScoApp::SetIncomingReadiness (bool readiness)
{
	LogMsg("SetIncomingReadiness = %d", readiness);

	BOOLEAN params = (BOOLEAN) readiness;
	unsigned long nbytes;
	if (!DeviceIoControl (hDevice, IOCTL_HFP_INCOMING_READINESS, &params, sizeof(params), 0, 0, &nbytes, 0))
		throw IntException (DialAppError_OpenScoFailure, "DeviceIoControl failed, GetLastError %d", GetLastError());
}


#pragma managed(pop)
