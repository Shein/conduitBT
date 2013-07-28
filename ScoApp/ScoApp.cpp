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
        /*
		KS: To think do we need it? May be to use FormatMessage for formatting all GetLastError?
		LPVOID lpMsgBuf = 0;

        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                          FORMAT_MESSAGE_FROM_SYSTEM |
                          FORMAT_MESSAGE_IGNORE_INSERTS,
                          0,
                          GetLastError(),
                          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                          (LPSTR) &lpMsgBuf,
                          0,
                          0
                          )) {

            printf("Error: %s", (LPSTR)lpMsgBuf);
            LocalFree(lpMsgBuf);
        }*/

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
		/*
        LPVOID lpMsgBuf = 0;

        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                         FORMAT_MESSAGE_FROM_SYSTEM |
                         FORMAT_MESSAGE_IGNORE_INSERTS,
                         0,
                         GetLastError(),
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                         (LPSTR) &lpMsgBuf,
                         0,
                         0)) {
            MessageBox(0, (LPCTSTR) lpMsgBuf, "Error", MB_OK);
            LocalFree(lpMsgBuf);
        }*/
        
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



/***********************************************************************************************\
									Public ScoApp methods
\***********************************************************************************************/

void ScoApp::Construct()
{
    InCall = false;
    LogMsg("HFP Device path: %s", DeviceInterfaceDetailData->DevicePath);
	OpenDriver();
	WaveOutDev = new WaveOut(this);
	WaveInDev  = new WaveIn(this);
}


void ScoApp::Destruct()
{
    if (hDevice && hDevice!=INVALID_HANDLE_VALUE)
		CloseHandle(hDevice);
	hDevice = 0;
	delete WaveOutDev;
	delete WaveInDev;
}


void ScoApp::OpenSco (uint64 destaddr)
{
	LogMsg("About to open SCO channel");

	if (!IsConstructed())
		throw IntException (DialAppError_InternalError, "ScoApp::Construct was not called");

	if (IsOpen())
		throw IntException (DialAppError_InternalError, "ScoApp::CloseSco was not called");

	if (!destaddr)
		throw IntException (DialAppError_InternalError, "Destination address cannot be null");

	unsigned long nbytes;
    if (!DeviceIoControl (hDevice, IOCTL_HFP_OPEN_SCO, &destaddr, sizeof(destaddr), 0, 0, &nbytes, NULL))
		throw IntException (DialAppError_OpenScoFailure, "Failed to open SCO, GetLastError %d", GetLastError());

	DestAddr = destaddr;

	WaveOutDev->Open();
	WaveInDev->Open();
}


void ScoApp::CloseSco ()
{
	LogMsg("About to close SCO channel");

	if (IsOpen()) {
		WaveOutDev->Close();
		WaveInDev->Close();
		ReopenDriver();
		InCall = false;
		DestAddr = 0;
	}
}


void ScoApp::VoiceStart ()
{
	LogMsg("About to start passing voice");
    InCall = true;
	WaveOutDev->Play();
	WaveInDev->Play();
}


#pragma managed(pop)
