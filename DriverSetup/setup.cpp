#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <stdlib.h>
#include <stdio.h>
#include <setupapi.h>
#include <regstr.h>
#include <infstr.h>
#include <cfgmgr32.h>
#include <string.h>
#include <malloc.h>
#include <newdev.h>
#include <objbase.h>

#include <Bluetoothapis.h>

#include "msg.h"
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

	if (FALSE == AdjustProcessPrivileges())
	{
		printf("Adjust Process Privileges failed\n");
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
		printf("BluetoothSetLocalServiceInfo failed, err = %d\n", err);
		return false;        
	}

	return true;
}


void FormatToStream(_In_ FILE * stream, _In_ DWORD fmt,...)
/*++

Routine Description:

    Format text to stream using a particular msg-id fmt
    Used for displaying localizable messages

Arguments:

    stream              - file stream to output to, stdout or stderr
    fmt                 - message id
    ...                 - parameters %1...

Return Value:

    none

--*/
{
    va_list arglist;
    LPTSTR locbuffer = NULL;
    DWORD count;

    va_start(arglist, fmt);
    count = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE|FORMAT_MESSAGE_ALLOCATE_BUFFER,
                          NULL,
                          fmt,
                          0,              // LANGID
                          (LPTSTR) &locbuffer,
                          0,              // minimum size of buffer
                          &arglist);

    if(locbuffer) {
        if(count) {
            int c;
            int back = 0;
            //
            // strip any trailing "\r\n"s and replace by a single "\n"
            //
            while(((c = *CharPrev(locbuffer,locbuffer+count)) == TEXT('\r')) ||
                  (c == TEXT('\n'))) {
                count--;
                back++;
            }
            if(back) {
                locbuffer[count++] = TEXT('\n');
                locbuffer[count] = TEXT('\0');
            }
            //
            // now write to apropriate stream
            //
            _fputts(locbuffer,stream);
        }
        LocalFree(locbuffer);
    }
}



int cmdUpdate(_In_ LPCTSTR BaseName, LPCTSTR inf ,LPCTSTR hwid)
/*++

Routine Description:
    UPDATE
    update driver for existing device(s)

Arguments:

    BaseName  - name of executable
    Machine   - machine name, must be NULL
    argc/argv - remaining parameters

Return Value:

    EXIT_xxxx

--*/
{
    HMODULE newdevMod = NULL;
    int failcode = EXIT_FAIL;
    BOOL reboot = FALSE;
    
    DWORD flags = 0;
    DWORD res;
    TCHAR InfPath[MAX_PATH];

    UNREFERENCED_PARAMETER(BaseName);
 

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
        //
        // inf doesn't exist
        //
        return EXIT_FAIL;
    }
    inf = InfPath;
    flags |= INSTALLFLAG_FORCE;

    FormatToStream(stdout,inf ? MSG_UPDATE_INF : MSG_UPDATE,hwid,inf);

    if(/*!UpdateFn*/!UpdateDriverForPlugAndPlayDevices(NULL,hwid,inf,flags,&reboot)) {

		printf("UpdateFn 0x%x\n", GetLastError());
        goto final;
    }

    FormatToStream(stdout,MSG_UPDATE_OK);

    failcode = reboot ? EXIT_REBOOT : EXIT_OK;

final:

    if(newdevMod) {
        FreeLibrary(newdevMod);
    }

    return failcode;
}


int cmdInstall(_In_ LPCTSTR BaseName)
/*++

Routine Description:

    CREATE
    Creates a root enumerated devnode and installs drivers on it

Arguments:

    BaseName  - name of executable
    Machine   - machine name, must be NULL
    argc/argv - remaining parameters

Return Value:

    EXIT_xxxx

--*/
{
    HDEVINFO DeviceInfoSet = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA DeviceInfoData;
    GUID ClassGUID;
    TCHAR ClassName[MAX_CLASS_NAME_LEN];
    TCHAR hwIdList[LINE_LEN+4];
    TCHAR InfPath[MAX_PATH];
    int failcode = EXIT_FAIL;
    LPCTSTR hwid = L"BTHENUM\\{c07508f2-b970-43ca-b5dd-cc4f2391bef4}";
    LPCTSTR inf	= L"HfpDriver.inf";

    //
    // Inf must be a full pathname
    //
    if(GetFullPathName(inf,MAX_PATH,InfPath,NULL) >= MAX_PATH) {
        //
        // inf pathname too long
        //
        return EXIT_FAIL;
    }
	
    //
    // List of hardware ID's must be double zero-terminated
    //
    ZeroMemory(hwIdList,sizeof(hwIdList));
    if (FAILED(StringCchCopy(hwIdList,LINE_LEN,hwid))) {
        goto final;
    }

    //
    // Use the INF File to extract the Class GUID.
    //
    if (!SetupDiGetINFClass(InfPath,&ClassGUID,ClassName,sizeof(ClassName)/sizeof(ClassName[0]),0))
    {
        goto final;
    }

    //
    // Create the container for the to-be-created Device Information Element.
    //
    DeviceInfoSet = SetupDiCreateDeviceInfoList(&ClassGUID,0);
    if(DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        goto final;
    }

    //
    // Now create the element.
    // Use the Class GUID and Name from the INF file.
    //
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    if (!SetupDiCreateDeviceInfo(DeviceInfoSet,
        ClassName,
        &ClassGUID,
        NULL,
        0,
        DICD_GENERATE_ID,
        &DeviceInfoData))
    {
        goto final;
    }

    //
    // Add the HardwareID to the Device's HardwareID property.
    //
    if(!SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
        &DeviceInfoData,
        SPDRP_HARDWAREID,
        (LPBYTE)hwIdList,
        (lstrlen(hwIdList)+1+1)*sizeof(TCHAR)))
    {
        goto final;
    }

    //
    // Transform the registry element into an actual devnode
    // in the PnP HW tree.
    //
    if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE,
        DeviceInfoSet,
        &DeviceInfoData))
    {
		printf("SetupDiCallClassInstaller failed: 0x%X\n", GetLastError());
        goto final;
    }

    FormatToStream(stdout,MSG_INSTALL_UPDATE);
    //
    // update the driver for the device we just created
    //
    failcode = cmdUpdate(BaseName, inf, hwid);

final:

    if (DeviceInfoSet != INVALID_HANDLE_VALUE) {
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    }

    return failcode;
}


int	__cdecl	_tmain(_In_ int /*argc*/, _In_reads_(argc) PWSTR* argv)
{
	LPCTSTR baseName;
	baseName = _tcsrchr(argv[0],TEXT('\\'));
	if(!baseName)
	{
		baseName = argv[0];
	} 
	else 
	{
		baseName = CharNext(baseName);
	}

	if(!SetBthServiceInfo(true))
	{
		return -1;
	}

	if(cmdInstall(baseName) >= EXIT_FAIL)
	{
		SetBthServiceInfo(false);
	}

	return 0;
}