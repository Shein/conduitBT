#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <stdlib.h>
#include <stdio.h>
#include <newdev.h>
#include <Bluetoothapis.h>

#include "setup.h"


BOOL AdjustProcessPrivileges()
{
	HANDLE procToken;
	LUID luid;
	TOKEN_PRIVILEGES tp;
	BOOL bRetVal;
	DWORD err;

	bRetVal = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &procToken);
	if (!bRetVal)
	{
		err = GetLastError();
		printf("OpenProcessToken failed, err = %d\n", err);
		goto exit;
	}

	bRetVal = LookupPrivilegeValue(NULL, SE_LOAD_DRIVER_NAME, &luid);
	if (!bRetVal)
	{
		err = GetLastError();
		printf("LookupPrivilegeValue failed, err = %d\n", err);
		goto exit1;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	//
	// AdjustTokenPrivileges can succeed even when privileges are not adjusted.
	// In such case GetLastError returns ERROR_NOT_ALL_ASSIGNED.
	//
	// Hence we check for GetLastError in both success and failure case.
	//

	(void) AdjustTokenPrivileges(procToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES) NULL, (PDWORD)NULL);
	err = GetLastError();

	if (err != ERROR_SUCCESS)
	{
		bRetVal = FALSE;
		printf("AdjustTokenPrivileges failed, err = %d\n", err);
		goto exit1;
	}

exit1:
	CloseHandle(procToken);
exit:
	return bRetVal;
}

bool SetBthServiceInfo(BOOLEAN bEnabled)
{
	DWORD err = ERROR_SUCCESS;
	BLUETOOTH_LOCAL_SERVICE_INFO SvcInfo = {0};
	SvcInfo.Enabled = bEnabled;
	GUID BTHECHOSAMPLE_SVC_GUID = { 0xc07508f2, 0xb970, 0x43ca, {0xb5, 0xdd, 0xcc, 0x4f, 0x23, 0x91, 0xbe, 0xf4} };
	PWSTR BthEchoSampleSvcName = L"BthEchoSampleSrv";

	if (!AdjustProcessPrivileges())
	{
		return false;
	}

	if (FAILED(StringCbCopyW(SvcInfo.szName, sizeof(SvcInfo.szName), BthEchoSampleSvcName)))
	{
		printf("Copying svc name failed\n");
		return false;
	}

	if (ERROR_SUCCESS != (err = BluetoothSetLocalServiceInfo(
		NULL, //callee would select the first found radio
		&BTHECHOSAMPLE_SVC_GUID,
		0,
		&SvcInfo
		)))
	{
		printf("Bluetooth Not Found (???) err = %d\n", err);
		return false;        
	}

	return true;
}


int cmdUpdate()
{
	int failcode = EXIT_FAIL;
	LPCTSTR hwid = L"BTHENUM\\{c07508f2-b970-43ca-b5dd-cc4f2391bef4}";
	LPCTSTR inf	= L"HfpDriver.inf";
    BOOL reboot = FALSE;
    
    DWORD flags = 0;
    DWORD res;
    TCHAR InfPath[MAX_PATH];

    //
    // Inf must be a full pathname
    //
    res = GetFullPathName(inf,MAX_PATH,InfPath,NULL);
    if((res >= MAX_PATH) || (res == 0)) {
        //
        // inf pathname too long
        //
        return EXIT_FAIL;
    }
    if(GetFileAttributes(InfPath)==(DWORD)(-1)) {
		printf("HfpDriver.inf Not found at %ls\n", (char*)InfPath); 
        return EXIT_FAIL;
    }
    inf = InfPath;
    flags |= INSTALLFLAG_FORCE;

	Sleep(500);
    if(!UpdateDriverForPlugAndPlayDevices(NULL,hwid,InfPath,flags,&reboot)) {
		int err = GetLastError();
		printf("Update Driver Failed. Error 0x%x\n", err);
		if(err == 2)
		{
			printf("Probably WdfCoinstaller01011.dll is missing\n");
		}
        goto final;
    }

    failcode = reboot ? EXIT_REBOOT : EXIT_OK;

final:

    return failcode;
}

int	__cdecl	_tmain()
{


	if(!SetBthServiceInfo(true))
	{
		return -1;
	}

	if(cmdUpdate() >= EXIT_FAIL)
	{
		return -2;
	}

	printf("Driver Installed Successfully\n");
	return 0;
}