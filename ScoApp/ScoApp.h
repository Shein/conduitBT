#pragma once
#pragma managed(push, off)

#include "def.h"
#include "deblog.h"
#include "Wave.h"


/*
 ************************************************************************************************
 C++ class for working with HFP Driver.
 All methods can throw char* exception with the error description (except the empty ScoApp() 
 constructor, which does nothing, and the destructor with Destruct() method).
 Construct() function is intenionaly separated from the constructor itself in order to simplify 
 exceptions handling.

 Usage: 
 1. To call once Init() to get driver's interface ID
 2. Create ScoApp object using its simple constructor
 3. Call the real constructor: Construct() method
 4. Call OpenSco() after the high level applications has already established a control connection
 ************************************************************************************************
 */
class ScoApp : public DebLog
{
  public:
    HANDLE  hDevice;	// May be tested for detecting the object constructing state
	UINT64	DestAddr;	// Address of a Destination Bluetooth device 

  public:
	static void Init ();
	static void End  ();

  public:
	ScoApp() : DebLog("ScoApp "), hDevice(0), DestAddr(0)
	{}

	~ScoApp()
	{
		Destruct();
	}

	void Construct ();
	void Destruct  () throw();
	void OpenSco   (uint64 destaddr);
	void CloseSco  ();

	void VoiceStart ();

	bool IsConstructed()	{ return (hDevice!=0);	}
	bool IsOpen ()			{ return (DestAddr!=0); }

  protected:
	void  OpenDriver ();
	void  ReopenDriver ();

  protected:
    static PSP_DEVICE_INTERFACE_DETAIL_DATA  DeviceInterfaceDetailData;

  protected:
	bool	 InCall;
	WaveOut	*WaveOutDev;
	WaveIn	*WaveInDev;
};

#define DMO_ENABLED

#pragma managed(pop)

