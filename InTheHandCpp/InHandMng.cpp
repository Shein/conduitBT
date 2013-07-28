/*
#include "def.h"
#include "InHand.h"
#include "InHandMng.h"


//static
int InHandMng::GetDevices(InHandDev* &devices)
{
	int	len = Devices->Length;
	if (len <= 0)
		return 0;

	devices = new InHandDev[len];

	int	i = 0;
	for each (BluetoothDeviceInfo^ item in Devices) {
		devices[i].Address = item->DeviceAddress->ToInt64();
		devices[i].Name	   = (wchar_t*) Marshal::StringToHGlobalUni(item->DeviceName).ToPointer();
		i++;
	}
	return len;
}


//static
void InHandMng::FreeDevices(InHandDev* &devices, int n)
{
	if (devices) {
		for (int i=0; i<n; i++)
			;//TODO - check it Marshal::FreeHGlobal((void*)devices[i].Name);

		delete[] devices;
		devices = 0;
	}
}
*/

